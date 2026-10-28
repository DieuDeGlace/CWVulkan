// Build-time GLSL -> SPIR-V compiler (glslang library, no Python needed).
// Usage: spirv_compile <input.vert|.frag> <output.h> <symbol> <include dir>
// Writes `static const uint32_t <symbol>[] = {...};`. `#include "x"` lines are
// expanded textually from the include directory.
#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/resource_limits_c.h>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

static bool readFile(const std::string &path, std::string &out) {
    std::ifstream in(path, std::ios::binary);
    if (!in)
        return false;
    std::stringstream ss;
    ss << in.rdbuf();
    out = ss.str();
    return true;
}

static bool expand(const std::string &source, const std::string &dir, std::string &out, int depth = 0) {
    std::istringstream lines(source);
    std::string line;
    while (std::getline(lines, line)) {
        if (line.rfind("#extension GL_GOOGLE_include_directive", 0) == 0) {
            out += "\n";
            continue;
        }
        if (line.rfind("#include", 0) == 0) {
            auto a = line.find('"'), b = line.rfind('"');
            std::string text;
            if (a == std::string::npos || b <= a || depth > 8 ||
                !readFile(dir + "/" + line.substr(a + 1, b - a - 1), text)) {
                std::fprintf(stderr, "cannot include: %s\n", line.c_str());
                return false;
            }
            if (!expand(text, dir, out, depth + 1))
                return false;
            continue;
        }
        out += line + "\n";
    }
    return true;
}

int main(int argc, char **argv) {
    if (argc != 5) {
        std::fprintf(stderr, "usage: spirv_compile input output symbol includedir\n");
        return 2;
    }
    std::string input = argv[1], raw, source;
    if (!readFile(input, raw) || !expand(raw, argv[4], source)) {
        std::fprintf(stderr, "cannot read %s\n", input.c_str());
        return 1;
    }
    glslang_stage_t stage = input.size() > 5 && input.substr(input.size() - 5) == ".frag"
                                ? GLSLANG_STAGE_FRAGMENT
                                : GLSLANG_STAGE_VERTEX;
    glslang_initialize_process();
    glslang_input_t in{};
    in.language = GLSLANG_SOURCE_GLSL;
    in.stage = stage;
    in.client = GLSLANG_CLIENT_VULKAN;
    in.client_version = GLSLANG_TARGET_VULKAN_1_3;
    in.target_language = GLSLANG_TARGET_SPV;
    in.target_language_version = GLSLANG_TARGET_SPV_1_6;
    in.code = source.c_str();
    in.default_version = 450;
    in.default_profile = GLSLANG_NO_PROFILE;
    in.messages = (glslang_messages_t)(GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT);
    in.resource = glslang_default_resource();
    glslang_shader_t *shader = glslang_shader_create(&in);
    if (!glslang_shader_preprocess(shader, &in) || !glslang_shader_parse(shader, &in)) {
        std::fprintf(stderr, "%s:\n%s\n", input.c_str(), glslang_shader_get_info_log(shader));
        return 1;
    }
    glslang_program_t *program = glslang_program_create();
    glslang_program_add_shader(program, shader);
    if (!glslang_program_link(program, in.messages)) {
        std::fprintf(stderr, "%s link:\n%s\n", input.c_str(), glslang_program_get_info_log(program));
        return 1;
    }
    glslang_program_SPIRV_generate(program, stage);
    size_t words = glslang_program_SPIRV_get_size(program);
    const unsigned *code = glslang_program_SPIRV_get_ptr(program);
    std::ofstream out(argv[2]);
    out << "#pragma once\n#include <cstdint>\nstatic const uint32_t " << argv[3] << "[] = {";
    for (size_t i = 0; i < words; i++)
        out << (i % 8 ? " " : "\n    ") << code[i] << "u,";
    out << "\n};\n";
    glslang_program_delete(program);
    glslang_shader_delete(shader);
    glslang_finalize_process();
    return out ? 0 : 1;
}
