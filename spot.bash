spot() {
  case $1 in
    find | list | ls | search)
      local LIST
      if LIST=$(command spot "$@"); then
        SPOTIFY_RESULTS=("${SPOTIFY_RESULTS[@]}" $(cut -f1 -s <<< "$LIST"))
        LESS=FRX less <<< "$LIST"
      else
        return $?
      fi
      ;;
    play)
      command spot cat "${@:2}" | ogg123 -q -
      ;;
    *)
      command spot "$@"
      ;;
  esac
}

__spot_complete() {
  if [ $COMP_CWORD -eq 1 ]; then
    COMPREPLY=($(compgen -W 'cat find get list ls play search' -- "$2"))
  else
    case ${COMP_WORDS[1]} in
      cat | get | list | ls | play)
        COMPREPLY=($(
          for WORD in "$2" "spotify:album:$2" "spotify:track:$2"; do
            compgen -W "${SPOTIFY_RESULTS[*]}" -- "$WORD"
          done
        ))
        ;;
    esac
  fi
}

complete -o filenames -F __spot_complete spot
