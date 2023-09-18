#include "DKUtil/Hook.hpp"

/*
* STARFIELD SFSE MOD
* Companions Never Lose Affinity.
*
* DESCRIPTION: Hook the affinity modifier write step so
* modifiers less than 0 become 0.
*
* HOOK POINTS:
* - Starfield.exe + 0x1878AB3
* - Starfield.exe + 0x2A324D0 [Deprecated, wasn't needed, caused issues]
*/

uintptr_t BASE_ADDRESS = DKUtil::Hook::Module::get().base();
using namespace dku::Alias;

template <std::size_t N, typename T = std::uint8_t>
std::unique_ptr<dku::Hook::ASMPatchHandle> createHook(
    uintptr_t targetOffset,
    int targetLength,
    std::array<T, N> opcode)
{
    /**
     * Creates an ASM patch object.
     * @param targetOffset: Offset to add to the base address to find the target location to patch.
     * @param targetLength: Length of the opcode we want to overwrite with this patch, e.g. 0x8 or 8.
     * @param opcode: New opcode to write with this patch.
     * @returns: ASMPatchHandle that can be enabled.
     */
    return dku::Hook::AddASMPatch(
        BASE_ADDRESS + targetOffset,
        std::make_pair(0x0, targetLength),
        { opcode.data(), opcode.size() });
}

namespace NoAffinityLossHook
{
    void Install()
    {
        // Make the first patch
        std::array<std::uint8_t, 19> opcode1{
            0x0F, 0x57, 0xC0, // Set register to 0
            0x0F, 0x2F, 0x41, 0x48, // Compare 0 to new value
            0x72, 0x05, // Jump to write if new value 0 or above
            0xC5, 0xFA, 0x11, 0x41, 0x48, // Write 0 if below
            0xC5, 0xFA, 0x10, 0x41, 0x48 // Write final number
        };
        auto affinityHook1 = createHook(0x1878AB3, 0x5, opcode1);
        affinityHook1->Enable();
        INFO("First area patched!");

    }
}

DLLEXPORT constinit auto SFSEPlugin_Version = []() noexcept {
	SFSE::PluginVersionData data{};

	data.PluginVersion(Plugin::Version);
	data.PluginName(Plugin::NAME);
	data.AuthorName(Plugin::AUTHOR);
	data.UsesSigScanning(true);
	//data.UsesAddressLibrary(true);
	data.HasNoStructUse(true);
	//data.IsLayoutDependent(true);
	data.CompatibleVersions({ SFSE::RUNTIME_LATEST });

	return data;
}();

namespace
{
	void MessageCallback(SFSE::MessagingInterface::Message* a_msg) noexcept
	{
		switch (a_msg->type) {
		case SFSE::MessagingInterface::kPostLoad:
			{
                // Install plugin hook
				NoAffinityLossHook::Install();
                break;
			}
		default:
			break;
		}
	}
}

/**
* For preload plugins:
void SFSEPlugin_Preload(SFSE::LoadInterface* a_sfse);
**/

DLLEXPORT bool SFSEAPI SFSEPlugin_Load(const SFSE::LoadInterface* a_sfse)
{
/**#ifndef NDEBUG
    // Current disabled due to infinite loop even on built release
	while (!IsDebuggerPresent()) {
		Sleep(100);
	}
#endif**/

	SFSE::Init(a_sfse, false);

	DKUtil::Logger::Init(Plugin::NAME, std::to_string(Plugin::Version));

	INFO("{} v{} loaded", Plugin::NAME, Plugin::Version);

	// Set up our plugin
	SFSE::AllocTrampoline(1 << 8);
	SFSE::GetMessagingInterface()->RegisterListener(MessageCallback);

	return true;
}
