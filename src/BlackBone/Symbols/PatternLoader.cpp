#include "PatternLoader.h"
#include "../Include/Winheaders.h"
#include "../Patterns/PatternSearch.h"
#include "../Misc/Trace.hpp"
#include <3rd_party/VersionApi.h>

#include <memory>
#include <unordered_map>

namespace blackbone
{

struct OffsetData
{
    PatternSearch pattern;
    bool bit64;
    int32_t functionOffset;
    int32_t dataStartOffset;
    int32_t dataOperandOffset;
    int32_t dataInstructionSize;
};

struct ScanParams
{
    ptr_t start = 0;
    ptr_t size = 0;
    ptr_t diff = 0;
};

void FindPattern( const ScanParams& scan32, const ScanParams& scan64, const OffsetData& rule, ptr_t& result )
{
    // Skip if already found
    if (result != 0)
    {
        return;
    }

    const ScanParams& scan = rule.bit64 ? scan64 : scan32;

    std::vector<ptr_t> found;
    rule.pattern.Search( reinterpret_cast<void*>(scan.start), static_cast<size_t>(scan.size), found );
    if (!found.empty())
    {
        // Plain pointer sum
        if (rule.functionOffset != -1)
        {
            result = found.front() - rule.functionOffset + scan.diff;
        }
        // Pointer dereference inside instruction
        else if (rule.dataStartOffset != 0)
        {
            if (rule.bit64)
            {
                result = *reinterpret_cast<int32_t*>(found.front() + (rule.dataStartOffset + rule.dataOperandOffset)) +
                    (found.front() + rule.dataStartOffset + rule.dataInstructionSize) + scan.diff;
            }
            else
            {
                result = *reinterpret_cast<int32_t*>(found.front() + rule.dataStartOffset);
            }
        }
    }
};

/// <summary>
/// Fill OS-dependent patterns
/// </summary>
/// <param name="patterns">Pattern collection</param>
/// <param name="result">Result</param>
void OSFillPatterns( std::unordered_map<ptr_t*, OffsetData>& patterns, SymbolData& result )
{
    if (IsWindows1124H2OrGreater()) {
        // LdrpHandleTlsData64
		patterns.emplace(&result.LdrpHandleTlsData64, OffsetData{ "\x4C\x8B\xDC\x49\x89\x5B\x10\x49\x89\x73\x18\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x81\xEC\x00\x01\x00\x00", true, 0 });

        // RtlInsertInvertedFunctionTable64
        patterns.emplace(&result.RtlInsertInvertedFunctionTable64, OffsetData{ "\x48\x8B\xC4\x48\x89\x58\x08\x48\x89\x68\x10\x48\x89\x70\x20\x57\x48\x83\xEC\x30\x83\x60\x18\x00", true, 0 });

        // RtlpInsertInvertedFunctionTableEntry64
        patterns.emplace(&result.LdrpInvertedFunctionTable64, OffsetData{ "\x8B\x0D\xAB\x61\x0F\x00", true, -1, -0xF, 2, 6 });

        // RtlInsertInvertedFunctionTable32
        patterns.emplace(&result.RtlInsertInvertedFunctionTable32, OffsetData{ "\x8B\xFF\x55\x8B\xEC\x83\xEC\x0C\x53\x56\x57\x8D\x45\xF8\x8B\xFA", false, 0 });

        // RtlpInsertInvertedFunctionTableEntry32 - look for  "mov     eax, ds:_LdrpInvertedFunctionTables"
        // 33 F6 46 3B C6
        patterns.emplace(&result.LdrpInvertedFunctionTable32, OffsetData{ "\x33\xF6\x46\x3B\xC6", false, -1, -0x1B });

        // LdrpHandleTlsData32
        patterns.emplace(&result.LdrpHandleTlsData32, OffsetData{ "\x8B\x4D\xB8\x33\xD2", false, 0x42 });

        // LdrProtectMrdata
        // 75 20 85 f6 75 35
        patterns.emplace(&result.LdrProtectMrdata, OffsetData{ "\x75\x20\x85\xf6\x75\x35", false, 0x1d });
    }
    else if (IsWindows1123H2OrGreater()) {
		// LdrpHandleTlsData64
        patterns.emplace(&result.LdrpHandleTlsData64, OffsetData{ "\x48\x89\x5C\x24\x10\x48\x89\x74\x24\x18\x48\x89\x7C\x24\x20\x41\x55", true, 0 });

        // RtlInsertInvertedFunctionTable64
        patterns.emplace(&result.RtlInsertInvertedFunctionTable64, OffsetData{ "\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x30\x8B\xDA", true, 0 });

        // RtlpInsertInvertedFunctionTableEntry64
        patterns.emplace(&result.LdrpInvertedFunctionTable64, OffsetData{ "\x48\x8B\xC4\x48\x89\x58\x08\x48\x89\x68\x10\x48\x89\x70\x18\x48\x89\x78\x20\x41\x56\x48\x83\xEC\x20\x8B\x05\x2D\xF3\x16\x00", true, 0 });

        // RtlInsertInvertedFunctionTable32
        patterns.emplace(&result.RtlInsertInvertedFunctionTable32, OffsetData{ "\x83\xEC\x0C\x53\x56\x57\x8D\x45\xF8\x8B\xFA", false, 0 });

        // RtlpInsertInvertedFunctionTableEntry32 - look for  "mov     eax, ds:_LdrpInvertedFunctionTables"
        // 33 F6 46 3B C6
        patterns.emplace(&result.LdrpInvertedFunctionTable32, OffsetData{ "\x33\xF6\x46\x3B\xC6", false, -1, -0x1B });

        // LdrpHandleTlsData32
        patterns.emplace(&result.LdrpHandleTlsData32, OffsetData{ "\x33\xF6\x85\xC0\x79\x03", false, 0x42 });

        // LdrProtectMrdata
        // 75 20 85 f6 75 35
        patterns.emplace(&result.LdrProtectMrdata, OffsetData{ "\x75\x20\x85\xf6\x75\x35", false, 0x1d });
    }
    else if (IsWindows1121H2OrGreater())
    {
        // LdrpHandleTlsData64
        // 41 55 41 56 41 57 48 81 EC F0 00 00
        patterns.emplace(&result.LdrpHandleTlsData64, OffsetData{ "\x41\x55\x41\x56\x41\x57\x48\x81\xEC\xF0\x00\x00", true, 0xf });

        // RtlInsertInvertedFunctionTable64
        // 48 89 5C 24 08 57 48 83 EC 30 8B DA
        patterns.emplace(&result.RtlInsertInvertedFunctionTable64, OffsetData{ "\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x30\x8B\xDA", true, 0 });

        // RtlpInsertInvertedFunctionTableEntry64
        // 49 8B E8 48 8B FA 0F 84
        patterns.emplace(&result.LdrpInvertedFunctionTable64, OffsetData{ "\x49\x8b\xe8\x48\x8b\xfa\x0f\x84", true, -1, -0xF, 2, 6 });

        // RtlInsertInvertedFunctionTable32
        // 53 56 57 8D 45 F8 8B FA
        patterns.emplace(&result.RtlInsertInvertedFunctionTable32, OffsetData{ "\x53\x56\x57\x8d\x45\xf8\x8b\xfa", false, 0x8 });

        // RtlpInsertInvertedFunctionTableEntry32
        // 33 F6 46 3B C6
        patterns.emplace(&result.LdrpInvertedFunctionTable32, OffsetData{ "\x33\xF6\x46\x3B\xC6", false, -1, -0x1B });

        // LdrpHandleTlsData32
        // 33 f6 85 c0 79 03
        auto offset = 0x2c;
        if (IsWindows1122H2OrGreater())
            offset = 0x42;

        patterns.emplace(&result.LdrpHandleTlsData32, OffsetData{ "\x33\xf6\x85\xc0\x79\x03", false, offset });

        // LdrProtectMrdata
        // 75 20 85 f6 75 35
        patterns.emplace(&result.LdrProtectMrdata, OffsetData{ "\x75\x20\x85\xf6\x75\x35", false, 0x1d });
    }
    else if (IsWindows10RS3OrGreater())
    {
        // LdrpHandleTlsData
        // 74 33 44 8D 43 09
        auto offset = 0x43;
        if(IsWindows1019H1OrGreater())
            offset = 0x46;
        else if (IsWindows10RS4OrGreater())
            offset = 0x44;

        patterns.emplace( &result.LdrpHandleTlsData64, OffsetData{ "\x74\x33\x44\x8d\x43\x09", true, offset } );

        // RtlInsertInvertedFunctionTable
        // 48 8D 54 24 58 48 8B F9 E8 - 20H1
        // 8B FA 49 8D 43 20 - RS3-19H2
        if (IsWindows1020H1OrGreater())
            patterns.emplace( &result.RtlInsertInvertedFunctionTable64, OffsetData{ "\x48\x8d\x54\x24\x58\x48\x8b\xf9\xe8", true, 0x11 } );
        else
            patterns.emplace( &result.RtlInsertInvertedFunctionTable64, OffsetData{ "\x8b\xfa\x49\x8d\x43\x20", true, 0x10 } );

        // RtlpInsertInvertedFunctionTableEntry
        // 49 8B E8 48 8B FA 0F 84
        patterns.emplace( &result.LdrpInvertedFunctionTable64, OffsetData{ "\x49\x8b\xe8\x48\x8b\xfa\x0f\x84", true, -1, -0xF, 2, 6 } );

        // RtlInsertInvertedFunctionTable
        // 53 56 57 8D 45 F8 8B FA
        patterns.emplace( &result.RtlInsertInvertedFunctionTable32, OffsetData{ "\x53\x56\x57\x8d\x45\xf8\x8b\xfa", false, 0x8 } );

        // RtlpInsertInvertedFunctionTableEntry
        // 33 F6 46 3B C6
        patterns.emplace( &result.LdrpInvertedFunctionTable32, OffsetData{ "\x33\xF6\x46\x3B\xC6", false, -1, -0x1B } );

        // LdrpHandleTlsData
        // 33 F6 85 C0 79 03 - RS5-20H1
        // 8B C1 8D 4D AC/BC 51 - RS3/RS4
        auto pattern = "\x8b\xc1\x8d\x4d\xbc\x51";
        if (IsWindows10RS5OrGreater())
            pattern = "\x33\xf6\x85\xc0\x79\x03";
        else if (IsWindows10RS4OrGreater())
            pattern = "\x8b\xc1\x8d\x4d\xac\x51";

        offset = 0x18;
        if (IsWindows1020H1OrGreater())
            offset = 0x2C;
        else if (IsWindows1019H1OrGreater())
            offset = 0x2E;
        else if (IsWindows10RS5OrGreater())
            offset = 0x2C;

        patterns.emplace( &result.LdrpHandleTlsData32, OffsetData{ pattern, false, offset } );

        // LdrProtectMrdata
        // 75 25 85 F6 75 08  - 20H1
        // 75 24 85 F6 75 08  - RS3-19H2
        if(IsWindows1020H1OrGreater())
            patterns.emplace( &result.LdrProtectMrdata, OffsetData{ "\x75\x25\x85\xf6\x75\x08", false, 0x1D } );
        else
            patterns.emplace( &result.LdrProtectMrdata, OffsetData{ "\x75\x24\x85\xf6\x75\x08", false, 0x1C } );
    }
    else if (IsWindows10RS2OrGreater())
    {
        // LdrpHandleTlsData
        // 74 33 44 8D 43 09
        patterns.emplace( &result.LdrpHandleTlsData64, OffsetData{ "\x74\x33\x44\x8d\x43\x09", true, 0x43 } );

        // RtlInsertInvertedFunctionTable
        // 8B FA 49 8D 43 20
        patterns.emplace( &result.RtlInsertInvertedFunctionTable64, OffsetData{ "\x8b\xfa\x49\x8d\x43\x20", true, 0x10 } );

        // RtlpInsertInvertedFunctionTableEntry
        // 49 8B E8 48 8B FA 0F 84
        patterns.emplace( &result.LdrpInvertedFunctionTable64, OffsetData{ "\x49\x8b\xe8\x48\x8b\xfa\x0f\x84", true, -1, -0xF, 2, 6 } );

        // RtlInsertInvertedFunctionTable
        // 8D 45 F0 89 55 F8 50 8D 55 F4
        patterns.emplace( &result.RtlInsertInvertedFunctionTable32, OffsetData{ "\x8d\x45\xf0\x89\x55\xf8\x50\x8d\x55\xf4", false, 0xB } );
        patterns.emplace( &result.LdrpInvertedFunctionTable32, OffsetData{ "\x8d\x45\xf0\x89\x55\xf8\x50\x8d\x55\xf4", false, -1, 0x4C } );

        // LdrpHandleTlsData
        // 8B C1 8D 4D BC 51
        patterns.emplace( &result.LdrpHandleTlsData32, OffsetData{ "\x8b\xc1\x8d\x4d\xbc\x51", false, 0x18 } );

        // LdrProtectMrdata
        // 75 24 85 F6 75 08
        patterns.emplace( &result.LdrProtectMrdata, OffsetData{ "\x75\x24\x85\xf6\x75\x08", false, 0x1C } );
    }
    else if (IsWindows8Point1OrGreater())
    {
        // LdrpHandleTlsData
        // 44 8D 43 09 4C 8D 4C 24 38
        patterns.emplace( &result.LdrpHandleTlsData64, OffsetData{ "\x44\x8d\x43\x09\x4c\x8d\x4c\x24\x38", true, 0x43 } );

        // RtlInsertInvertedFunctionTable
        // 8B C3 2B D3 48 8D 48 01
        patterns.emplace( &result.RtlInsertInvertedFunctionTable64, OffsetData{ "\x8b\xc3\x2b\xd3\x48\x8d\x48\x01", true, 0x84 } );
        patterns.emplace( &result.LdrpInvertedFunctionTable64, OffsetData{ "\x8b\xc3\x2b\xd3\x48\x8d\x48\x01", true, -1, -0x27, 3, 7 } );

        // RtlInsertInvertedFunctionTable
        // 53 56 57 8B DA 8B F9 50
        patterns.emplace( &result.RtlInsertInvertedFunctionTable32, OffsetData{ "\x53\x56\x57\x8b\xda\x8b\xf9\x50", false, 0xB } );
        patterns.emplace( &result.LdrpInvertedFunctionTable32, OffsetData{ "\x53\x56\x57\x8b\xda\x8b\xf9\x50", false, -1, IsWindows10OrGreater() ? 0x22 : 0x23 } );

        // LdrpHandleTlsData
        // 50 6A 09 6A 01 8B C1
        patterns.emplace( &result.LdrpHandleTlsData32, OffsetData{ "\x50\x6a\x09\x6a\x01\x8b\xc1", false, 0x1B } );

        // LdrProtectMrdata
        // 83 7D 08 00 8B 35
        patterns.emplace( &result.LdrProtectMrdata, OffsetData{ PatternSearch( "\x83\x7d\x08\x00\x8b\x35", 6 ), false, 0x12 } );
    }
    else if (IsWindows8OrGreater())
    {
        // LdrpHandleTlsData
        // 48 8B 79 30 45 8D 66 01
        patterns.emplace( &result.LdrpHandleTlsData64, OffsetData{ "\x48\x8b\x79\x30\x45\x8d\x66\x01", true, 0x49 } );

        // RtlInsertInvertedFunctionTable
        // 8B FF 55 8B EC 51 51 53 57 8B 7D 08 8D
        patterns.emplace( &result.RtlInsertInvertedFunctionTable32, OffsetData{ "\x8b\xff\x55\x8b\xec\x51\x51\x53\x57\x8b\x7d\x08\x8d", false, 0 } );
        patterns.emplace( &result.LdrpInvertedFunctionTable32, OffsetData{ "\x8b\xff\x55\x8b\xec\x51\x51\x53\x57\x8b\x7d\x08\x8d", false, -1, 0x26 } );

        // LdrpHandleTlsData
        // 8B 45 08 89 45 A0
        patterns.emplace( &result.LdrpHandleTlsData32, OffsetData{ "\x8b\x45\x08\x89\x45\xa0", false, 0xC } );
    }
    else if (IsWindows7OrGreater())
    {
        const bool update1 = WinVer().revision > 24059;

        // LdrpHandleTlsData
        // 41 B8 09 00 00 00 48 8D 44 24 38
        patterns.emplace( &result.LdrpHandleTlsData64, OffsetData{ PatternSearch( "\x41\xb8\x09\x00\x00\x00\x48\x8d\x44\x24\x38", 11 ), true, update1 ? 0x23 : 0x27 } );

        // LdrpFindOrMapDll patch address
        // 48 8D 8C 24 98/90 00 00 00 41 B0 01
        auto pattern = update1 ? "\x48\x8D\x8C\x24\x90\x00\x00\x00\x41\xb0\x01" : "\x48\x8D\x8C\x24\x98\x00\x00\x00\x41\xb0\x01";
        patterns.emplace( &result.LdrKernel32PatchAddress, OffsetData{ PatternSearch( pattern, 11 ), true, -0x12 } );

        // KiUserApcDispatcher patch address
        // 48 8B 4C 24 18 48 8B C1 4C
        patterns.emplace( &result.APC64PatchAddress, OffsetData{ "\x48\x8b\x4c\x24\x18\x48\x8b\xc1\x4c", true, 0 } );

        // RtlInsertInvertedFunctionTable
        // 8B FF 55 8B EC 56 68
        patterns.emplace( &result.RtlInsertInvertedFunctionTable32, OffsetData{ "\x8b\xff\x55\x8b\xec\x56\x68", false, 0 } );

        // RtlLookupFunctionTable + 0x11
        // 89 5D E0 38
        patterns.emplace( &result.LdrpInvertedFunctionTable32, OffsetData{ "\x89\x5D\xE0\x38", false, -1, 0x1B } );

        // LdrpHandleTlsData
        // 74 20 8D 45 D4 50 6A 09 
        patterns.emplace( &result.LdrpHandleTlsData32, OffsetData{ "\x74\x20\x8d\x45\xd4\x50\x6a\x09", false, 0x14 } );
    }
}

/// <summary>
/// Scan ntdll for internal loader data
/// </summary>
/// <param name="ntdll32">Mapped x86 ntdll</param>
/// <param name="ntdll64">Mapped x64 ntdll</param>
/// <param name="result">Result</param>
/// <returns>Status code</returns>
NTSTATUS ScanSymbolPatterns( const pe::PEImage& ntdll32, const pe::PEImage& ntdll64, SymbolData& result )
{
    ScanParams scan32, scan64;

    scan32.diff = static_cast<int64_t>(ntdll32.imageBase()) - reinterpret_cast<int64_t>(ntdll32.base());
    scan64.diff = static_cast<int64_t>(ntdll64.imageBase()) - reinterpret_cast<int64_t>(ntdll64.base());

    auto fillRanges = []( const pe::PEImage& file, ScanParams& params )
    {
        for (const auto& sec : file.sections())
        {
            if (_stricmp( reinterpret_cast<const char*>(sec.Name), ".text" ) == 0)
            {
                params.start = reinterpret_cast<ptr_t>(file.base()) + sec.VirtualAddress;
                params.size = sec.Misc.VirtualSize;
                break;
            }
        }
    };

    // Get code section bounds
    fillRanges( ntdll32, scan32 );
    if (ntdll64.base())
    {
        fillRanges( ntdll64, scan64 );
    }  

    std::unordered_map<ptr_t*, OffsetData> patterns;
    OSFillPatterns( patterns, result );

    // Final search
    for (const auto& e : patterns)
    {
        FindPattern( scan32, scan64, e.second, *e.first );
    }

    // Retry with old patterns
    if (result.RtlInsertInvertedFunctionTable32 == 0 && IsWindows8Point1OrGreater() && !IsWindows10RS2OrGreater())
    {
        // RtlInsertInvertedFunctionTable
        // 8D 45 F4 89 55 F8 50 8D 55 FC
        OffsetData rule1{ "\x8d\x45\xf4\x89\x55\xf8\x50\x8d\x55\xfc", false, 0xB };
        OffsetData rule2{ "\x8d\x45\xf4\x89\x55\xf8\x50\x8d\x55\xfc", false, -1, 0x1D };

        FindPattern( scan32, scan64, rule1, result.RtlInsertInvertedFunctionTable32 );
        FindPattern( scan32, scan64, rule2, result.LdrpInvertedFunctionTable32 );
    }

    // Report errors
#ifndef BLACKBONE_NO_TRACE
    if (result.LdrpHandleTlsData64 == 0)
        BLACKBONE_TRACE( "PatternData: LdrpHandleTlsData64 not found" );
    if (result.LdrpHandleTlsData32 == 0)
        BLACKBONE_TRACE( "PatternData: LdrpHandleTlsData32 not found" );
    if (IsWindows8Point1OrGreater() && result.LdrpInvertedFunctionTable64 == 0)
        BLACKBONE_TRACE( "PatternData: LdrpInvertedFunctionTable64 not found" );
    if (result.LdrpInvertedFunctionTable32 == 0)
        BLACKBONE_TRACE( "PatternData: LdrpInvertedFunctionTable32 not found" );
    if (IsWindows8Point1OrGreater() && result.RtlInsertInvertedFunctionTable64 == 0)
        BLACKBONE_TRACE( "PatternData: RtlInsertInvertedFunctionTable64 not found" );
    if (result.RtlInsertInvertedFunctionTable32 == 0)
        BLACKBONE_TRACE( "PatternData: RtlInsertInvertedFunctionTable32 not found" );
    if (IsWindows8Point1OrGreater() && result.LdrProtectMrdata == 0)
        BLACKBONE_TRACE( "PatternData: LdrProtectMrdata not found" );
    if (IsWindows7OrGreater() && !IsWindows8OrGreater())
    {
        if (result.LdrKernel32PatchAddress == 0)
            BLACKBONE_TRACE( "PatternData: LdrKernel32PatchAddress not found" );
        if (result.APC64PatchAddress == 0)
            BLACKBONE_TRACE( "PatternData: APC64PatchAddress not found" );
    }
#endif

    return STATUS_SUCCESS;
}

}