#ifndef A3064641_9766_4FAA_8DF7_2EC97A00AF86
#define A3064641_9766_4FAA_8DF7_2EC97A00AF86

class Identifier;

struct VariantMacro final {
    std::string identifier;
    unsigned long line;
    unsigned value;
};

struct VariantUniform final {
    std::string identifier;
    GLint location;
};

class ShaderProgram final {
public:
    bool setup(const Identifier& id);
    bool update(void);
    void destroy(void);

    std::size_t add_uniform(std::string name);
    void set_variant_vert(unsigned variant, unsigned value);
    void set_variant_frag(unsigned variant, unsigned value);

    std::string vert_path;
    std::string frag_path;
    std::vector<std::string> vert_lines;
    std::vector<std::string> frag_lines;
    std::vector<VariantMacro> vert_macros;
    std::vector<VariantMacro> frag_macros;
    std::vector<VariantUniform> uniforms;
    bool needs_update;
    GLuint handle;
};

#endif /* A3064641_9766_4FAA_8DF7_2EC97A00AF86 */
