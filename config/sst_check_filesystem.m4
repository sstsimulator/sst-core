AC_DEFUN([SST_CHECK_FILESYSTEM],
[
  AC_LANG_SAVE
   AC_LANG([C++])
  # In older versions of gcc implementation of std::filesystem is kept
  # in separate stdc++fs library. You should link it explicitly
  AC_MSG_CHECKING([if std::filesystem requires linking stdc++fs])
  AC_LINK_IFELSE(
      [AC_LANG_SOURCE([
          #include <filesystem>
          int main() {
              std::filesystem::create_directory("/dev/null");
          }
      ])],
      [ac_cv_fs_stdlib=no],
      [ac_cv_fs_stdlib=yes]
  )
  if test "x$ac_cv_fs_stdlib" = xyes; then
      AC_MSG_RESULT(yes)
      LIBS="$LIBS -lstdc++fs"
  else
      AC_MSG_RESULT(no)
  fi
  AC_LANG_RESTORE

])
