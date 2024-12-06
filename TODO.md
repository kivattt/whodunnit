- File picker on the right?

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
