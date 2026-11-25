#ifndef DFFB28CF_26D9_40A5_8D5D_27DDEEAD8C3F
#define DFFB28CF_26D9_40A5_8D5D_27DDEEAD8C3F

struct MoveMode final {
    constexpr static unsigned FLY = 0;
    constexpr static unsigned WALK = 1;

    static void register_component(void);

    std::uint8_t value;
};

#endif /* DFFB28CF_26D9_40A5_8D5D_27DDEEAD8C3F */
