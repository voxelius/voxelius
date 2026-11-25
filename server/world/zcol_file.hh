#ifndef E7C4A19B_3D52_4F86_9B07_5A1C2E8D6F34
#define E7C4A19B_3D52_4F86_9B07_5A1C2E8D6F34

#include "server/constant.hh"

class ReadBuffer;
class WriteBuffer;

class ZcolFile final {
public:
    explicit ZcolFile(std::filesystem::path path);

    ZcolFile(const ZcolFile& other) = delete;
    ZcolFile& operator=(const ZcolFile& other) = delete;

    ZcolFile(ZcolFile&& other) noexcept = default;
    ZcolFile& operator=(ZcolFile&& other) noexcept = default;

    ~ZcolFile(void);

    bool is_valid(void) const;

    bool read_slot(std::size_t slot, ReadBuffer& buffer);
    void write_slot(std::size_t slot, const WriteBuffer& buffer);

private:
    static std::size_t header_size(void);

    bool load_header(void);
    void save_header(void);

    void build_free_ranges(void);
    std::optional<std::uint64_t> take_free_range(std::uint64_t size);
    void insert_free_range(std::uint64_t offset, std::uint64_t length);

    std::filesystem::path m_path;
    std::fstream m_stream;
    bool m_valid { false };

    std::array<std::uint64_t, constant::REGION_AREA> m_offsets;
    std::array<std::uint64_t, constant::REGION_AREA> m_lengths;
    std::array<std::uint64_t, constant::REGION_AREA> m_checksums;

    std::vector<std::pair<std::uint64_t, std::uint64_t>> m_free_ranges;
};

#endif /* E7C4A19B_3D52_4F86_9B07_5A1C2E8D6F34 */
