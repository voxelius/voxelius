#ifndef A1BE1728_FBEC_44DF_B4FF_D36FBD21BFFA
#define A1BE1728_FBEC_44DF_B4FF_D36FBD21BFFA

#include "core/cvar/number.hh"

#include "client/gui/setting.hh"

namespace gui
{
template<typename T>
class Slider final : public SettingBuilder<Slider<T>> {
public:
    Slider<T>& bind(std::string_view name);
    Slider<T>& set_range(T min, T max);
    Slider<T>& set_format(std::string_view format);
    Slider<T>& set_step(T step);

    T value(void) const;
    void set_value(T value);

protected:
    virtual void layout_control(void) override;
    virtual void translate_control(void) override;

private:
    cvar::Number<T> m_value {};
    T m_min {};
    T m_max {};
    T m_step {};
    std::string m_format {};
};
} // namespace gui

namespace gui
{
using SliderInt = Slider<int>;
using SliderFloat = Slider<float>;
using SliderUnsigned = Slider<unsigned>;
} // namespace gui

#endif /* A1BE1728_FBEC_44DF_B4FF_D36FBD21BFFA */
