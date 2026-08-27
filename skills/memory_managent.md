# SKILL: MEMORY MANAGER

[KEYWORDS: remember, save, memory, recall, search, forget, database, store, long-term]

## 1. PURPOSE
This skill instructs you (the Agent) on when and how to use the `memory_save` and `memory_search` tools to manage long-term information.

## 2. CORE RULES
- If the user explicitly tells you to "remember", "save for later", or states an important personal fact/preference, use `TOOL_CALL: memory_save(key: value)`.
- If the user asks a question about their past preferences or facts you should know, use `TOOL_CALL: memory_search(keyword)` before trying to answer.
- Structure your `memory_save` arguments clearly so they are easy to search later.
- Do not rely on your pre-trained knowledge if the user asks about something they specifically told you to remember.

## 3. FEW-SHOT EXAMPLES

[Example 1: Saving Important Technical Notes]
User: "Remember that for my DSA project, the head pointer of the LinkedList must always be initialized to NULL."
- Thought: The user wants me to store a specific programming rule for long-term recall. I will use the memory_save tool.
TOOL_CALL: memory_save(LinkedList Rule: head pointer must be initialized to NULL)

[Example 2: Recalling Information]
User: "What was the rule I told you about the LinkedList?"
- Thought: The user is asking about a previously saved fact. I need to query the database using a relevant keyword.
TOOL_CALL: memory_search(LinkedList)

User: Tool result: "LinkedList Rule: head pointer must be initialized to NULL"
Continue. If done write FINAL_ANSWER: ...
- Thought: I have retrieved the information from memory and can now answer the user.
FINAL_ANSWER: You told me that the head pointer of the LinkedList must always be initialized to NULL.