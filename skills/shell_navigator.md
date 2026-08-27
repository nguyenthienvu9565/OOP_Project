# SKILL: SHELL NAVIGATOR

[KEYWORDS: shell, terminal, bash, linux, directory, file, grep, search, find, command, exec]

## 1. PURPOSE
This skill guides you (the Agent) on how to effectively and safely use the `exec` tool to navigate the Linux operating system, inspect files, and search for specific information within a terminal environment.

## 2. CORE RULES
- When asked to find a specific string or hidden information inside files, use `grep` via the `exec` tool.
- If you don't know the exact name or location of a file, use `exec(ls -la)` or `exec(find . -name "pattern")` to explore the directory tree first.
- Keep the shell commands simple and standard (e.g., `cat`, `ls`, `grep`, `pwd`).
- Only output the exact string format required for the action. Do not add markdown backticks around the tool call.

## 3. FEW-SHOT EXAMPLES

[Example 1: Exploring Directory Before Acting]
User: "Find out what files are in the secret folder and read the configuration."
- Thought: I need to see the contents of the 'secret' folder first to know the exact filename of the configuration file.
TOOL_CALL: exec(ls -la secret/)

[Example 2: Searching for Specific Strings]
User: "Search for the hidden flag format 'CTF{...}' inside the log.txt file."
- Thought: The user wants to extract a specific string pattern from a text file. Using the grep command is the most efficient way to do this.
TOOL_CALL: exec(grep "CTF{" log.txt)