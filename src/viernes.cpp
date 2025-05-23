#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include <regex>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

// Safe function to get HOME
std::string get_home_dir() {
    const char* home = getenv("HOME");
    return (home ? std::string(home) : "/data/data/com.termux/files/home");
}

// Configuration
const std::string API_URL       = "https://jotalea.com.ar/api/gemini.php";
const std::string API_KEY       = "IAmAHorriblePersonAndIAmStealingSomeonesData";
const fs::path    CONFIG_DIR    = get_home_dir() + "/.friday";
const fs::path    HISTORY_FILE  = CONFIG_DIR / "history.log";

// Result structure
struct CommandResult {
    std::string command;
    std::string message;
    bool valid;
};

// Callback to receive CURL response
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Extract JSON from Markdown blocks, AI will very likely ignore the prompt to not use them
std::string extractJson(const std::string& raw) {
    std::regex jsonBlock(R"(```json\s*(\{.*\})\s*```)", std::regex::ECMAScript); // | std::regex::dotall); // this doesn't seem useful
    std::smatch match;
    if (std::regex_search(raw, match, jsonBlock)) {
        return match[1].str();
    }
    return raw;
}

// Get system context
std::string get_system_context() {
    std::stringstream ctx;
    const char* user = getenv("USER");
    ctx << "User: " << (user ? user : "unknown") << "\n";
    ctx << "CWD: "  << fs::current_path().string() << "\n";
    ctx << "Files: ";
    int count = 0;
    for (auto& e : fs::directory_iterator(fs::current_path())) {
        if (++count > 5) break;
        ctx << e.path().filename().string() << " ";
    }
    return ctx.str();
}

// Dynamically generate the prompt
std::string build_prompt(const std::string& query) {
    std::ostringstream p;
    p << "Escribí un comando de shell SÓLO para esto: " << query << "\n"
      << "Respondé SÓMO en ESTE formato JSON: {\"command\":\"[COMANDO]\",\"message\":\"[EXPLICACIÓN]\"}\n"
      << "Usá \"&&\" para encadenar comandos; no uses múltiples líneas en el output. NO uses Markdown, porque ROMPE el parser del lado del cliente. Sé breve. Si no sabés qué hacer, respondé con {\"command\":\"null\"}.\n"
      << "Contexto: " << get_system_context();
    return p.str();
}

// Parse the JSON response
CommandResult parse_response(const json& api_resp) {
    CommandResult res{"", "", false};
    try {
        std::string raw = api_resp.dump();
        // Clear Markdown
        std::string clean = extractJson(raw);
        auto j = json::parse(clean);

        // {"response": "..."}
        auto resp_str = j.at("response").get<std::string>();

        std::string inner = extractJson(resp_str);
        auto obj = json::parse(inner);

        res.command = obj.value("command", "");
        res.message = obj.value("message", "");
        res.valid   = (!res.command.empty() && res.command != "null");
    } catch (const std::exception& e) {
        std::cerr << "Error al procesar el JSON.\n";
    }
    return res;
}

// Call the API, attach @file if present
CommandResult ask_ai(std::string query) {
    std::smatch m;
    std::string file_path;
    if (std::regex_search(query, m, std::regex("@([^\\s]+)"))) {
        file_path = m[1].str();
        query = std::regex_replace(query, std::regex("@[^\\s]+"), "");
    }

    CURL* curl = curl_easy_init();
    std::string resp_str;
    CommandResult result;

    if (!curl) return result;

    curl_mime* mime = curl_mime_init(curl);
    curl_mimepart* part;

    part = curl_mime_addpart(mime);
    curl_mime_name(part, "key");
    curl_mime_data(part, API_KEY.c_str(), CURL_ZERO_TERMINATED);

    std::string prompt = build_prompt(query);
    part = curl_mime_addpart(mime);
    curl_mime_name(part, "prompt");
    curl_mime_data(part, prompt.c_str(), CURL_ZERO_TERMINATED);

    if (fs::exists(HISTORY_FILE)) {
        std::ifstream hfile(HISTORY_FILE);
        std::ostringstream buf;
        buf << hfile.rdbuf();
        part = curl_mime_addpart(mime);
        curl_mime_name(part, "history");
        curl_mime_data(part, buf.str().c_str(), CURL_ZERO_TERMINATED);
    }

    if (!file_path.empty() && fs::exists(file_path)) {
        part = curl_mime_addpart(mime);
        curl_mime_name(part, "files");
        curl_mime_filedata(part, file_path.c_str());
    }

    struct curl_slist* hdrs = nullptr;
    hdrs = curl_slist_append(hdrs, "Accept: application/json");

    curl_easy_setopt(curl, CURLOPT_URL,        API_URL.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_setopt(curl, CURLOPT_MIMEPOST,   mime);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp_str);

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        try {
            json j = json::parse(resp_str);
            result = parse_response(j);
        } catch (...) {
            std::cerr << "La API devolvió un JSON inválido.\n";
        }
    } else {
        std::cerr << "Error de cURL: " << curl_easy_strerror(res) << "\n";
    }

    curl_slist_free_all(hdrs);
    curl_mime_free(mime);
    curl_easy_cleanup(curl);

    return result;
}

// Ask for confirmation and run
void execute_command(const CommandResult& cmd) {
    std::cout << "\nComando sugerido:\n" << cmd.command << "\n";
    if (!cmd.message.empty()) {
        std::cout << "Explicación: " << cmd.message << "\n";
    }
    std::cout << "\n¿Ejecutar? [s/N]: ";
    std::string c; std::getline(std::cin, c);
    if (c == "s" || c == "S" || c == "y" || c == "Y") {
        system(cmd.command.c_str());
    } else {
        std::cout << "Cancelado.\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <optionalmente, adjuntar un @archivo>\n";
        return 1;
    }

    try {
        if (!fs::exists(CONFIG_DIR)) {
            fs::create_directories(CONFIG_DIR);
        }
        if (!fs::exists(HISTORY_FILE)) {
            std::ofstream(HISTORY_FILE) << "[]";
        }
    } catch (const std::exception& e) {
        std::cerr << "Error creando la config: " << e.what() << "\n";
        return 1;
    }

    std::ostringstream q;
    for (int i = 1; i < argc; ++i) {
        q << argv[i] << " ";
    }

    auto result = ask_ai(q.str());
    if (result.valid) {
        execute_command(result);
        return 0;
    }

    std::cerr << "No se pudo generar un comando válido.\n";
    return 1;
}
