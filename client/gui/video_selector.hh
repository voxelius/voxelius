#ifndef D1EFD2D7_28CA_49AD_8D74_DD542D9D9E5B
#define D1EFD2D7_28CA_49AD_8D74_DD542D9D9E5B

#include <mutex>

#include "client/gui/setting.hh"

namespace gui
{
class VideoSelector final : public SettingBuilder<VideoSelector> {
public:
    using mode_type = std::optional<SDL_DisplayMode>;
    using decision_type = std::optional<bool>;

    VideoSelector& on_change(std::function<decision_type(const mode_type&)> callback);

protected:
    virtual void layout_control(void) override;
    virtual void translate_control(void) override;

private:
    void ensure_choices(void);
    void apply_mode(const mode_type& mode);

    std::once_flag m_choices_once {};
    std::vector<SDL_DisplayMode> m_choices {};
    std::vector<std::string> m_choice_labels {};

    bool m_pending { false };
    mode_type m_pending_mode {};
    std::function<decision_type(const mode_type&)> m_callback {};
};
} // namespace gui

#endif /* D1EFD2D7_28CA_49AD_8D74_DD542D9D9E5B */
