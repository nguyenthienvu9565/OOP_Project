#include "Agent_Core_New_Update.h"
#include "Harness.h"
#include <print>
#include <memory>

// A trivial mock tool so we have something for the factory to create.
class WebSearchTool : public Tool {
public:
    std::string getName() const override { return "WebSearch"; }
    std::optional<std::string> execute(const std::string& input) override {
        return "Kết quả tìm kiếm giả lập cho: " + input;
    }
};

int main() {
    auto registry = std::make_shared<ToolRegistry>();
    registry->registerTool("WebSearch", [] { return std::make_unique<WebSearchTool>(); });

    auto agent = std::make_shared<AgentLoop>(registry);

    HarnessRunner harness(agent);

    // At least 2 evaluators, as required.
    harness.addEvaluator(std::make_shared<KeywordEvaluator>(
        std::vector<std::string>{"hoàn thành", "dữ liệu"}, /*match_all=*/true));

    harness.addEvaluator(std::make_shared<FunctionalEvaluator>(
        "HasAtLeastOneAction",
        [](const TrajectoryRecord& t) {
            for (const auto& step : t.steps) {
                if (step.type == "ACTION") return true;
            }
            return false;
        }));

    std::vector<Task> tasks = {
        {"task-1", "Tìm kiếm giá cổ phiếu Anthropic"},
        {"task-2", "Tìm kiếm thời tiết hôm nay"},
    };

    auto results = harness.runBatch(tasks);

    std::println("\n=== Success rate: {:.2f} ===", harness.calculateSuccessRate(results));

    harness.exportToJson(results, "batch_results.json");
    std::println("Kết quả đã được export ra batch_results.json");

    return 0;
}