- Line numbers
- File picker on the right?

- Author name text alignment (it is slightly too high up)
- Smooth scrolling, better scrolling, mouse-acceleration style scrolling

- Git log buttons:
    - Show full history
    - Hide commits not shown in blame (hide shadowed commits) (hide invisible commits) "overshadowed by a later commit"
    - Show tooltips, also on red commits (question mark button on the right of them?) in git log explaining they are "overshadowed by a later commit, not shown in the currently visible git blame"

- Look into the `git blame --incremental` flag
- Look into the `git blame --encoding=...` flag for encoding author names and stuff

- Don't use system()
```cpp
string run_git_blame(string filename) {
	pid_t pid = fork();
	if (pid < 0) {
		return ""; // TODO: Errors
	}

	if (pid > 0) {
		int status;
		waitpid(pid, &status, 0);
		return "";
	}

	if (chdir(dir(filename)) != 0) { // TODO: Parent dir
		return "";
	}

	const char *args[] = {"blame", "--line-porcelain", "-t", filename, nullptr};
	if (execvp("git", args) == -1) {
		return "";
	}

	char *const args[] = {}
}
```
