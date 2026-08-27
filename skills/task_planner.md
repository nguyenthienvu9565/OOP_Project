# SKILL: TASK PLANNING

[KEYWORDS: plan, step, multiple, complex, sequence, coordinate, first]

## 1. PURPOSE
This skill guides you (the Agent) on how to handle complex tasks by breaking them down into smaller, logical steps before executing any actions.

## 2. CORE RULES
- Always adhere to the ReAct (Observe -> Thought -> Action) loop.
- When presented with a complex request, your thought MUST be a numbered, step-by-step plan.
- Execute only ONE tool per step. You MUST use the exact format: `TOOL_CALL: tool_name(arguments)`.
- Do not use JSON. Do not add extra formatting to the tool call.
- Wait for the "Tool result" before moving to the next step of your plan.
- When the entire task is finished, you MUST use the format: `FINAL_ANSWER: your final conclusion or result`.

## 3. FEW-SHOT EXAMPLES

[Example 1: Research and Save]
User: "Find the current population of Tokyo and save it to tokyo.txt"
- Thought: To fulfill this task, I need a plan. Step 1: Use web_search to find the population of Tokyo. Step 2: Use write_file to save the retrieved number. I will begin with Step 1.
TOOL_CALL: web_search(current population of Tokyo)

[Example 2: Read, Compute, and Memorize]
User: "Read formula.txt, calculate it, and save the result to memory."
- Thought: I need a three-step plan. Step 1: read_file. Step 2: calculator. Step 3: memory_save. I will start with Step 1.
TOOL_CALL: read_file(formula.txt)