# SKILL: OSINT & DATA EXTRACTION PIPELINE

[KEYWORDS: extract, find all, email, phone, scrape, regex, osint, ctf, flag, deep search]

## 1. PURPOSE
This skill guides you (the Agent) on how to chain multiple tools to extract highly specific information from the internet. You will use a 3-step pipeline: Web Search -> Fetch URL -> Regex Search.

## 2. CORE RULES
- **Step 1:** If the user asks for specific data from a website but doesn't provide the exact link, use `TOOL_CALL: web_search(query)` to find the precise URL.
- **Step 2:** Once you have a target URL (starting with http:// or https://), use `TOOL_CALL: fetch_url(url)` to scrape the raw plain text from that webpage.
- **Step 3:** To accurately extract specific patterns (like emails, IP addresses, or CTF flags) from the scraped text, use `TOOL_CALL: regex_search(pattern|text)`.
- **Regex Rule:** Always use standard C++ ECMAScript regex syntax. Remember to separate the pattern and the text with a pipe `|`.
- Do not combine multiple tool calls in one step. Wait for the "Tool result" before proceeding to the next step.

## 3. FEW-SHOT EXAMPLES

[Example 1: CTF Flag Extraction]
User: "Find the hidden CTF flag on the challenge page: http://ctf.local/challenge1"
- Thought: I already have the URL. I need to fetch the content of this page first.
TOOL_CALL: fetch_url(http://ctf.local/challenge1)

User: Tool result: "Welcome hackers! The server is running. Here is your token: CTF{h1dd3n_w3b_fl4g_2026}."
Continue. If done write FINAL_ANSWER: ...
- Thought: I have the raw text. I will use regex_search to extract the exact flag format CTF{...}.
TOOL_CALL: regex_search(CTF\{[^}]+\}|Welcome hackers! The server is running. Here is your token: CTF{h1dd3n_w3b_fl4g_2026}.)

User: Tool result: "Found 1 match(es): 1. CTF{h1dd3n_w3b_fl4g_2026}"
Continue. If done write FINAL_ANSWER: ...
- Thought: The regex tool successfully extracted the flag. I can now present the final answer.
FINAL_ANSWER: The hidden flag is CTF{h1dd3n_w3b_fl4g_2026}.

[Example 2: Email OSINT]
User: "Find the contact email address of the Faculty of Information Technology on their website."
- Thought: I don't have the exact URL, so I must search the web first to find the IT Faculty's contact page.
TOOL_CALL: web_search(Faculty of Information Technology contact page)