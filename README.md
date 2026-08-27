# OOP_Project - Modular ReAct AI Agent Framework (6 Layers)

Dự án AI Agent đa tầng hoàn chỉnh viết bằng C++23.

## Cấu trúc 6 Tầng Hệ Thống

- **Tầng 1 (LLM Client)**: [`src/LLM_Client/`](file:///Users/nguyentrongtruc/OOP_Project/src/LLM_Client) - Giao tiếp với Ollama LLM server.
- **Tầng 2 (Tool Registry & Tools)**: [`src/tools/`](file:///Users/nguyentrongtruc/OOP_Project/src/tools) - Quản lý và thực thi 11 công cụ (Calculator, DateTime, Weather, File, WebSearch, Exec, Memory...).
- **Tầng 3 (Skill System)**: [`src/skills/`](file:///Users/nguyentrongtruc/OOP_Project/src/skills) - Nạp và quản lý các kỹ năng (Skills) từ các file Markdown.
- **Tầng 4 & 5 (Agent Loop & Loop Detector)**: [`src/agent/`](file:///Users/nguyentrongtruc/OOP_Project/src/agent) - Khung ReAct Agent Loop và bộ tự động phát hiện vòng lặp (Loop Detector).
- **Tầng 6 (Harness & Evaluator)**: [`src/harness/`](file:///Users/nguyentrongtruc/OOP_Project/src/harness) - HarnessRunner, Ghi vết Trajectory, Chấm điểm bài test (KeywordEvaluator & FunctionalEvaluator), Chạy Batch Evaluation và xuất kết quả dạng JSON.

---

## Hướng Dẫn Biên Dịch & Chạy Bằng CMake

```bash
# Tạo thư mục build và chuyển vào
mkdir -p build && cd build

# Cấu hình bằng CMake
cmake ..

# Biên dịch dự án
make -j

# Chạy bài test tích hợp 6 Tầng và Batch Evaluation
./run_eval
```

Kết quả evaluation sẽ tự động được xuất ra file `eval_results.json`.