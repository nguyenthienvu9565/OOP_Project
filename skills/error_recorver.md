# SKILL: ERROR RECOVERY

[KEYWORDS: error, fail, not found, syntax, exception, denied, cannot, wrong]

## 1. PURPOSE
This skill instructs you (the Agent) on how to handle situations where a Tool execution returns an error, fails, or produces unexpected results.

## 2. CORE RULES
- DO NOT immediately use `FINAL_ANSWER` to apologize or give up when encountering an error.
- ALWAYS analyze the error message in the "Tool result" to understand what went wrong.
- If `read_file` returns "File not found", assume the path might be wrong. Use `TOOL_CALL: exec(ls -la)` to inspect the current working directory.
- If `calculator` returns a syntax error, review your mathematical expression and remove any non-numeric characters.
- Limit your retry attempts to a maximum of 3. If it still fails, explain the root cause using `FINAL_ANSWER: ...`.
- ALWAYS strictly output either `TOOL_CALL: tool_name(args)` or `FINAL_ANSWER: text`.

## 3. FEW-SHOT EXAMPLES

[Example 1: File Not Found Recovery]
User: Tool result: Error: 'data.csv' not found.
Continue. If done write FINAL_ANSWER: ...
- Thought: The file 'data.csv' does not exist in the current directory. I should check what files are actually here using the shell.
TOOL_CALL: exec(ls -la)

[Example 2: Fixing Calculator Syntax]
User: Tool result: Error: Invalid syntax '15 * 17 USD'
Continue. If done write FINAL_ANSWER: ...
- Thought: The calculator failed because I included text ('USD'). I need to strip out non-numeric characters and try again.
TOOL_CALL: calculator(15 * 17)