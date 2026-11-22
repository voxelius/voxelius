#include "shared/pch.hh"

#include "shared/splash.hh"

#include "core/identifier.hh"
#include "core/utils/physfs.hh"

#include "shared/mod_context.hh"
#include "shared/mod_loader.hh"

static std::mt19937_64 s_random;
static std::vector<std::string> s_lines;

static std::string sanitize_line(std::string_view line)
{
    std::string result;
    result.reserve(line.size());

    for(const auto character : line) {
        if(character == 0x0A || character == 0x0D)
            continue;
        result.push_back(character);
    }

    return result;
}

static void append_lines(std::string_view name_space, std::string_view filename)
{
    auto id = Identifier::from_parts(name_space, filename);
    auto path = id.as_file_path("misc", ".txt");

    std::string line;
    std::istringstream stream;

    if(utils::read_file(path, stream)) {
        while(std::getline(stream, line)) {
            s_lines.emplace_back(sanitize_line(line));
        }
    }
}

void splash::init(std::string_view filename)
{
    for(const auto& ctx : mod_loader::all()) {
        append_lines(ctx.name_space(), filename);
    }

    if(s_lines.empty()) {
        s_lines.emplace_back("Nothing!");
    }

    s_random.seed(std::random_device {}());
}

std::string_view splash::get(void)
{
    if(s_lines.empty()) {
        return {};
    }
    else {
        std::uniform_int_distribution<std::size_t> dist(0, s_lines.size() - 1);

        return s_lines.at(dist(s_random));
    }
}
