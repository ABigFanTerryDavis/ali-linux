# ALI safe-rm - accident guard against nuking the system.
# NOT a security boundary: `command rm` or /bin/rm bypass it by design.
# It only stops typos like `rm -rf /` in interactive shells.
rm() {
  case " $* " in
    *" / "*|*" /* "*|*" --no-preserve-root "*)
      echo "ali-safe-rm: refused - that would wipe the system. Be specific." >&2
      return 1
      ;;
  esac
  command rm "$@"
}
