#!/usr/bin/perl

use v5.10.0;
use strict;
use warnings qw(FATAL all);
use Cwd qw(realpath);
use File::Basename;
use File::Find;
use File::Spec::Functions qw(abs2rel);
use File::Temp;
use Getopt::Long;
use List::Util qw(max);
use Pod::Usage;
use Text::Wrap;

$SIG{__DIE__} = sub { print STDERR @_; exit 2 };

# The regular expression used to detect GCC missing #include file warnings
my $regex = qr/^(?:(?:[+ |]*\+#include\s*(?<header><[^>]+>)|.*?(?:did you forget to|probably fixable by adding) ‘#include (?<header><[^>]+>)’))$/;

# The g++ compiler flags used to test files
my $cxxflags = "-fsyntax-only -std=c++17";

my ($root, $fix, $quiet, $nosort, $gcc, $errors);

get_options();
find_gcc();
find_and_check_files();
print_summary();

# Find the g++ compiler and confirm that it produces the correct output
sub find_gcc {
    my $gcc_found;
    my $tmp = File::Temp->new(SUFFIX => ".cc");
    print $tmp "std::vector<int> x;\n";
    close $tmp;
    for my $gcc_val ("g++", map "g++-$_", reverse 8..18) {
        if (open my $comp, "-|", "$gcc_val $cxxflags '$tmp' 2>&1") {
            $gcc_found //= $gcc_val;
            while (<$comp>) {
                if (/$regex/ && $+{header} eq "<vector>") {
                    $gcc = $gcc_val;
                    print STDERR `$gcc --version` if !$quiet;
                    print STDERR "Warning: GCC versions prior to 10 may not find all missing #includes\n"
                        if int(`$gcc -dumpversion`) < 10;
                    return;
                }
            }
        }
    }
    if ($gcc_found) {
        print STDERR `$gcc_found --version`, wrap("", "", <<EOF)

GCC command has been found as $gcc_found but it is not producing the expected warning output for missing #include <...> files.

This might require modifying \$regex in $0 to recognize slightly different GCC output.
EOF
    }
    gcc_error();
}

sub gcc_error {
    print STDERR wrap("", "", <<'EOF');

GCC needs to be installed, and g++ or g++-<nnn> needs to be in the $PATH.

This tool uses GCC-specific warnings which are not available on clang.

These features require GCC version 8 or later. GCC versions prior to 10 may not find all missing #includes.
EOF

    print STDERR wrap("", "", <<'EOF') if $^O =~ /darwin/i;

On MacOS, g++ is usually aliased to clang, but g++-<nnn> may be available.

/usr/local/bin may need to appear before /usr/bin in $PATH so that a homebrew-installed GCC can be found.
EOF

    exit 2;
}

# Whether a file is ignored by Git
sub check_ignore {
    state %check_ignore;
    my $path = realpath $_[0];
    return $check_ignore{$path} if exists $check_ignore{$path};
    exit 2 if -257 & system qw(git check-ignore -q --), $path;
    return $check_ignore{$path} = $? == 0;
}

# Scan C/C++ files in @ARGV
# Scan .h files first, in case --fix is enabled and fixing the .h
# file will automatically fix the corresponding other files
sub find_and_check_files {
    for my $suffix (qw(.h .hpp .c .cc .cpp)) {
        my $filter = sub {
            if (-d) {
                # prune .git, external, and directories which are ignored
                $File::Find::prune = 1 if /^\.git$/ || /^external$/ || check_ignore($_);
            } elsif (-f) {
                # check files with extension which are not ignored
                &check_file if /\Q$suffix\E$/ && !check_ignore($_);
            }
        };
        find($filter, $_) for @ARGV;
    }
}

sub print_summary {
    print STDERR "\n" if !$quiet;
    if ($errors) {
        print '-' x 119, "\n";
        if ($fix) {
            print "Missing #includes found and fixed in $errors files.\n";
        } else {
            die <<EOF;
Missing #includes found in $errors files.

To automatically fix these errors in a Git working tree, run:

$0 --fix
EOF
        }
    } else {
        print "No missing #includes found\n\nALL TESTS PASSED\n";
    }
    exit;
}

# Get the command-line options
sub get_options {
    $quiet = !-t STDERR;  # quiet by default if STDERR is not a terminal
    GetOptions(
        help => sub { pod2usage(-verbose => 2, -noperldoc => 1, -exitval => 0) },
        quiet => \$quiet,
        fix => \$fix,
        nosort => \$nosort,
        ) or pod2usage(-verbose => 1, -noperldoc => 1, -exitval => 2);
    chomp($root = `git rev-parse --show-toplevel`);
    exit 2 if $?;
    unshift @ARGV, $root if !@ARGV;
}

# Open a temporary source file with $suffix, editing $file, removing #include "..."
# lines and replacing #include "$header" with #include "$hdr"
#
# Returns an object which stringifies to filename and deletes it when destroyed
sub open_temp_src {
    my ($file, $suffix, $headers, $header, $hdr) = @_;
    my @headers;
    my $tmp = File::Temp->new(SUFFIX => $suffix);
    open my $f, "<", $file or die "Internal error: Cannot open $file: $!\n";
    while (<$f>) {
        if (my ($inc) = /^\s*#\s*include\s*"([^"]+)"/) {
            # Project #include "..." headers
            # Leave out $include "..." so that we can detect missing #includes
            next;
        } elsif (my ($sysinc) = /^\s*#\s*include\s*(<[^>]+>)/) {
            # System #include <...> headers
            $$headers{$sysinc} = 1;

            # Leave out #include <...> so that we can detect missing #includes
            next;
        }
        print $tmp $_;
    }
    close $tmp;
    return $tmp;
}

# Check a source file
sub check_file {
    my $file     = $_;
    my $fullpath = realpath($file);
    my $dir      = dirname($fullpath);

    my ($suffix) = /(\.(?:[^.]+))$/
        or die "Internal error: Cannot determine file suffix: $fullpath\n";

    # Progress dots
    print STDERR '.' if !$quiet;

    (my $header = $file) =~ s/\Q$suffix\E$/.h/;

    # determine language to force if file extension has ambiguous language
    my $lang = "";
    if ($suffix eq ".h" || $suffix eq ".c") {
        open my $f, "<", $fullpath or die "Cannot open $fullpath: $!\n";
        $lang = "-x c";
        while (<$f>) {
            s/\s//g;
            if (/\bstd::/ || /^#include<\w+>/ || /^template<.*>/ ||
                /^(class|virtual|using|namespace)\b/ || /^(public|private|protected):/) {
                $lang = "-x c++";
                last;
            }
        }
    }

    TRY: for my $tries (0..4) {
        my ($hdr, $src, %headers);

        if ($suffix ne ".h") {
            # If this is not a header file, create a modified header file of
            # the same prefix if it exists, and #include the modified header
            # file in the modified C/C++ file.
            $hdr = open_temp_src($header, ".h", \%headers) if -e $header;
            $src = open_temp_src($file, $suffix, \%headers, $header, $hdr);
        } else {
            # If this a header file, create a modified header file
            $src = open_temp_src($header, ".h", \%headers);
        }

        -e "$src" or die "Internal error: File $src does not exist\n";
        -e "$hdr" or die "Internal error: File $hdr does not exist\n" if $hdr;

        # GCC command to generate warnings about missing headers
        my $cmd = "$gcc $lang $cxxflags '$src' 2>&1";

        # Parse the GCC output
        open my $comp, "-|", $cmd or die "Could not execute $cmd: $!\n";
        my (@src, %edits);

        LINE: while (<$comp>) {
            s:$hdr:$dir/$header: if $hdr;
            s:$src:$dir/$file:;
            push @src, $_;

            # A +#include <...> at the beginning on a line indicates a missing
            # system header which needs to be added
            if (/$regex/) {
                my $inc = $+{header};
                # Don't handle the same missing #include twice in the same file
                if (!exists $edits{$inc}) {
                    # Find the filename GCC is complaining about
                    for (@src[max($#src-4, 0) .. $#src]) {
                        if (my ($editfile) = /^([^:]+):\d+:\d+: \w/) {
                            # Skip the edit file if it is not under $root
                            if (substr(abs2rel($editfile, $root), 0, 3) ne "../" &&
                                realpath($editfile) eq $fullpath) {
                                $edits{$inc} = sprintf "\n" . "-" x 119 . "\n%s\n%s",
                                    $fullpath, join "", @src[max($#src-5, 0) .. $#src];
                            }
                            next LINE;
                        }
                    }
                    print STDERR "Internal error: GCC output not recognized:\n",
                        @src[max($#src-5, 0) .. $#src];
                    gcc_error();
                }
            }
        }

        # Delete the temporary files
        undef $hdr;
        undef $src;

        # Remove edits with headers which were included with #include <...>
        delete @edits{keys %headers};

        # Return if there are no edits
        return if !%edits;

        # Increment error count on try 0
        ++$errors if !$tries;

        # Print the lines of compiler output of the #include warning
        print values %edits;

        # Return if we are not fixing files
        return if !$fix;

        # The contents of the source file
        my @prog = do {
            open my $fh, "<", $fullpath
                or die "Internal error: Cannot open $fullpath: $!\n";
            <$fh>
        };

        # The list of #include <...> which need to be added to the file
        my @inc = map { "#include $_\n" } keys %edits;

        # Scan the program, building @new from @prog and @inc
        # Several scans are made with different matching criteria,
        # and the first match wins
        my @new;

        # Put #include before first #include<> line if it exists
        if (@inc) {
            undef @new;
            for (@prog) {
                if (@inc) {
                    # Ignore #if/#ifdef blocks
                    unless (/^\s*#\s*if(def)?\b/ .. /^\s*#\s*endif\b/) {
                        @inc = splice @new, @new, 0, @inc if /^\s*#\s*include\s*</;
                    }
                }
                push @new, $_;
            }
        }

        # Put #include after the last #include in the file
        if (@inc) {
            my $insert;
            for my $i (0..$#prog) {
                local $_ = $prog[$i];
                # Ignore #if/#ifdef blocks
                unless (/^\s*#\s*if(def)?\b/ .. /^\s*#\s*endif\b/) {
                    # bail out early if certain declarations occur
                    last if /^\s*((class|struct|namespace|template)\b|extern\s*"C")/;
                    $insert = $i+1 if /^\s*#\s*include\b/;
                }
            }
            if (defined $insert) {
                @new = @prog;
                @inc = splice @new, $insert, 0, "\n", @inc;
            }
        }

        # Put #include before first class/struct/namespace/extern declaration
        if (@inc) {
            undef @new;
            for (@prog) {
                if (@inc) {
                    # Ignore #if/#ifdef blocks
                    unless (/^\s*#\s*if(def)?\b/ .. /^\s*#\s*endif\b/) {
                        @inc = splice @new, @new, 0, @inc, "\n"
                            if /^\s*(class|struct|namespace|template|extern)\b/;
                    }
                }
                push @new, $_;
            }
        }

        # Put #include after any comments at the beginning of the file
        if (@inc) {
            undef @new;
            for (@prog) {
                if (@inc) {
                    unless ((m:^\s*/\*: .. m:\*/\s*$:) || m:^\s*//:) {
                        @inc = splice @new, @new, 0, "\n", @inc;
                        push @new, "\n" if /\S/;
                    }
                }
                push @new, $_;
            }
        }

        # Sort contiguous blocks of #include <>
        if (!$nosort) {
            # Sort #includes, putting sst_config.h first
            sub byname {
                $a =~ /\bsst_config\.h\b/ ? -1 : $b =~ /\bsst_config\.h\b/ ? 1 : $a cmp $b;
            }

            my $start;
            for (my $i = 0; $i <= $#new; $i++) {
                if ($new[$i] =~ /^\s*#\s*include\s*<[^>]+>/) {
                    $start //= $i;
                } elsif (defined $start) {
                    @new[$start .. $i-1] = sort byname @new[$start .. $i-1];
                    undef $start;
                }
            }
            @new[$start .. $#new] = sort byname @new[$start .. $#new] if defined $start;
        }

        # Output the new file
        open my $fh, ">", $fullpath
            or die "Internal error: Cannot open $fullpath: $!\n";
        print $fh @new;
    }

    die <<EOF;

Too many repeated edits of

$fullpath

Something is not working to eliminate the errors.

This may occur if the only #include in a file is guarded by #ifdef/#if, and
this script cannot figure out where to insert the new #include. Moving it to
outside the #ifdef/#if may fix the error and provide a clear insertion point
for future runs of this script.

This script adds #includes next to existing #includes, so adding one for the
first time should make it work.

EOF
}

=pod

=head1 NAME

test-includes.pl - Test C/C++ sources for system #include files necessary to compile each source file without depending on other source files #include'ing the required headers.

=head1 SYNOPSIS

test-includes.pl [ options ] [ path... ]

=head1 OPTIONS

=over

=item B<--fix>

Fix the files, adding suggested #include lines. Changes will need to be reviewed and tested, and added to a Git commit.

=item B<--nosort>

Do not sort the #include<> directives in lexical order

=item B<--quiet>

Do not print progress dots on terminals

=item B<--help>

This help message

=back

=head1 DESCRIPTION

Looks at C/C++ files in the specified paths, looking for missing #include <> directives for system headers.

For example, if std::vector is used in a file, then #include <vector> should appear.

The .git and external directories, and any files/directories ignored by Git, are not scanned.

If no paths are specified, the root directory of the Git respository in the current working directory is used.

=head1 RETURN VALUE

=over

=item 0 on success

=item 1 if there are missing #includes

=item >1 if errors occurred during testing and/or fixing

=back

=head1 LIMITATIONS

GCC needs to be installed and g++ or g++-<nnn> needs to be in the PATH. This tool uses GCC-specific warnings which are not available on Clang.

=cut
