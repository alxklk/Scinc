# A shell function that runs t2fc and, if Options > System > "Go to the panel's
# folder on exit" is on, leaves the shell in the folder of the panel that was
# active when t2fc ended (as Midnight Commander's mc-wrapper.sh does: a
# program cannot change its shell's folder, so t2fc writes the folder to the file
# $T2FC_PWD_FILE names, and the function goes there).
#
# Source it from ~/.bashrc or ~/.zshrc:
#     . /path/to/Scinc/examples/claude/t2fc/t2fc-cd.sh
# It runs the t2fc found in PATH (a launcher such as ~/bin/t2fc that starts
# "Scinc t2fc.cpp" from this folder), passing the arguments on.
t2fc() {
	local f d s
	f=$(mktemp "${TMPDIR:-/tmp}/t2fc-pwd.XXXXXX") || return
	T2FC_PWD_FILE="$f" command t2fc "$@"
	s=$?
	if [ -s "$f" ]; then
		d=$(cat "$f")
		if [ -d "$d" ] && [ "$d" != "$PWD" ]; then
			cd -- "$d" || true
		fi
	fi
	rm -f "$f"
	return $s
}
