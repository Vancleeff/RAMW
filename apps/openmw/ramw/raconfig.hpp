#ifndef RAMW_RACONFIG_HPP
#define RAMW_RACONFIG_HPP

namespace RAMW
{
    class RAConfig
    {
    public:
        static constexpr bool TEST_MODE = true;

        static constexpr const char* TEST_USERNAME = "TestPlayer";
        static constexpr const char* TEST_USER_ICON = "textures/tx_goldicon.dds";

        // requires RAWeb local server for testing
        static constexpr const char* API_BASE_URL = "http://localhost:64000";

        static constexpr int CONNECTION_TIMEOUT_SECONDS = 10;
        static constexpr int HEARTBEAT_INTERVAL_SECONDS = 120;

        static inline const std::vector<std::string> TEST_ICON_POOL = {
            "icons/k/combat_block.dds",
            "icons/k/combat_heavyarmor.dds",
            "icons/k/combat_axe.dds",
            "icons/k/combat_spear.dds",
            "icons/k/combat_athletics.dds",
            "icons/k/magic_enchant.dds",
            "icons/k/magic_alteration.dds",
            "icons/k/magic_illusion.dds",
            "icons/k/magic_conjuration.dds",
            "icons/k/magic_mysticism.dds",
            "icons/k/magic_restoration.dds",
            "icons/k/magic_alchemy.dds",
            "icons/k/stealth_security.dds",
            "icons/k/stealth_acrobatics.dds",
            "icons/k/stealth_shortblade.dds",
            "icons/k/stealth_marksman.dds",
            "icons/k/stealth_mercantile.dds",
            "icons/k/stealth_handtohand.dds",
        };
    };
}

#endif
