#!/usr/bin/perl

use strict;
use warnings qw(FATAL all);
use Cwd qw(realpath);
use File::Basename;
use File::Find;
use File::Spec::Functions qw(abs2rel);
use File::Temp;
use Getopt::Long;
use List::Util qw(max);
use Memoize;
use Pod::Usage;

my ($root, $fix, $quiet);

get_options();

# Confirm the presence of g++ compiler
die <<EOF if `g++ --version`, $?;

GCC needs to be installed and g++ needs to be in the PATH.

This tool uses GCC-specific warnings which are not available on Clang.
EOF

memoize(qw(check_ignore NORMALIZER realpath));
find_and_check_files();
print_summary();

# Whether a file is ignored by Git
sub check_ignore {
    exit 2 if -257 & system qw(git check-ignore -q --), $_[0];
    return $? == 0;
}

# Scan C/C++ files starting from $root
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
        find($filter, $root);
    }
}

my $errors = 0;
sub print_summary {
    print STDERR "\n" if !$quiet;
    if ($errors) {
        if ($fix) {
            print "\nMissing #includes found and fixed in $errors files.\n";
        } else {
            print "\nMissing #includes found in $errors files.\n";
            exit 1;
        }
    } else {
        print "\nNo missing #includes found\n\nALL TESTS PASSED\n";
    }
    exit 0;
}

# Get the command-line options
sub get_options {
    $quiet = !-t STDERR;  # quiet by default if STDERR is not a terminal
    GetOptions(
        help => sub { pod2usage(-verbose => 2, -noperldoc => 1, -exitval => 0) },
        quiet => \$quiet,
        fix => \$fix,
        ) or pod2usage(-verbose => 1, -noperldoc => 1, -exitval => 2);
    if (@ARGV) {
        $root = realpath($ARGV[0]);
    } else {
        chomp($root = `git rev-parse --show-toplevel`);
        exit 2 if $?;
    }
    die qq(No such directory "$root"\n) if !-d $root;
}

# Open a temporary source file with $suffix, editing $file, removing #include "..."
# lines and replacing #include "$header" with #include "$hdr"
#
# Returns an object which stringifies to filename and deletes it when destroyed
sub open_temp_src {
    my ($file, $suffix, $header, $hdr) = @_;
    my $tmp = File::Temp->new(SUFFIX => $suffix);
    open my $f, "<", $file or die "Internal error: Cannot open $file: $!\n";
    while (<$f>) {
        if (my ($inc) = /^\s*#\s*include\s*"([^"]*)"/) {
            if (defined($hdr) && basename($inc) eq $header) {
                print $tmp qq(#include "$hdr"\n) ;
            } else {
                print $tmp "\n";
            }
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

    for my $tries (0..4) {
        my ($hdr, $src);

        # If this is not a header file, create a modified header file of the
        # same prefix if it exists, and #include the modified header file in
        # the modified C/C++ file.
        #
        # If this a header file, create a modified header file
        if ($suffix ne ".h") {
            $hdr = open_temp_src($header, ".h") if -e $header;
            $src = open_temp_src($file, $suffix, $header, $hdr);
        } else {
            $src = open_temp_src($header, ".h");
        }

        -e "$src" or die "Internal error: File $src does not exist\n";
        -e "$hdr" or die "Internal error: File $hdr does not exist\n" if $hdr;

        # GCC command to generate warnings about missing headers
        my $cmd = "g++ $lang -S -o /dev/null '$src' 2>&1";

        # Parse the GCC output
        open my $gcc, "-|", $cmd or die "Could not execute $cmd: $!\n";
        my (@src, %hit, %edits);

        LINE: while (<$gcc>) {
          s:$hdr:$dir/$header: if $hdr;
          s:$src:$dir/$file:;
            push @src, $_;

            # A +++ at the beginning of a line followed by an #include indicates
            # A missing system header which needs to be added
            if (my ($inc) = /^\s*\+\+\+ \|\+(#include <[^>]+>)$/) {
                $inc .= "\n";
                # Don't handle the same missing #include twice in the same file
                if (!$hit{$inc}++) {
                    # Find the filename GCC is complaining about and mark it with
                    # the #include, merging duplicates
                    for (@src[max($#src-4, 0) .. $#src]) {

                        if (my ($editfile) = /^([^:]+):\d+:\d+: \w/) {
                            # skip the edit file if it is not under $root
                            next LINE if substr(abs2rel($editfile, $root), 0, 3) eq "../";
                            next LINE if realpath($editfile) ne $fullpath;
                            ++$errors if !$tries && !exists $edits{$editfile};
                            if (!$edits{$editfile}{$inc}++) {
                                # Print the lines of compiler output of the #include warning
                                print "\n", "-" x 119, "\n$fullpath\n",
                                    @src[max($#src-5, 0) .. $#src];
                            }
                            next LINE;
                        }
                    }
                    die "Internal error: GCC output not recognized:\n",
                        @src[max($#src-5, 0) .. $#src];
                }
            }
        }

        return if !$fix or !grep keys %{$edits{$_}}, keys %edits;

        # Remove the temporary files
        undef $hdr;
        undef $src;

        # Go through all files marked for editing
        for my $editfile (keys %edits) {
            # The list of #include <...> which need to be added to the file
            my @inc = keys %{$edits{$editfile}};

            # The complete file to edit
            my @prog = do {
                open my $fh, "<", $editfile
                    or die "Internal error: Cannot open $editfile: $!\n";
                <$fh>
            };

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

            # Sort contiguous blocks of #include <...>
            my $start;
            for (my $i = 0; $i <= $#new; $i++) {
                if ($new[$i] =~ /^\s*#\s*include\s*</) {
                    $start //= $i;
                } elsif (defined $start) {
                    @new[$start .. $i-1] = sort @new[$start .. $i-1];
                    undef $start;
                }
            }
            @new[$start .. $#new] = sort @new[$start .. $#new] if defined $start;

            # Output the new file
            open my $fh, ">", $editfile
                or die "Internal error: Cannot open $editfile: $!\n";
            print $fh @new;
        }
    }

    print STDERR <<EOF;

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
    exit 2;
}

=pod

=head1 NAME

test-includes.pl - Test C/C++ sources for system #include files necessary to compile each source file without depending on other source files #include'ing the required headers.

=head1 SYNOPSIS

test-includes.pl [ options ] [ rootdir ]

=head1 OPTIONS

=over

=item B<--fix>

Fix the files, adding suggested #include lines. Changes will need to be reviewed and tested, and added to a Git commit.

=item B<--quiet>

Do not print progress dots on terminals

=item B<--help>

This help message

=back

=head1 DESCRIPTION

Looks at C/C++ files in the root directory and its subdirectories, looking for missing #include <> directives for system headers.

For example, if std::vector is used in a file, then #include <vector> should appear.

The .git and external directories, and any files/directories ignored by Git, are not scanned.

If no root directory is specified, the root directory of the Git respository in the current working directory is used.

=head1 RETURN VALUE

=over

=item 0 on success

=item 1 if there are missing #includes

=item >1 if errors occurred during testing and/or fixing

=back

=head1 LIMITATIONS

GCC needs to be installed and g++ needs to be in the PATH. This tool uses GCC-specific warnings which are not available on Clang.

=cut
