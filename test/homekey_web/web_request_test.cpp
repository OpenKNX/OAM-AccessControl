#include "OpenKNX/Network/Webserver/WebRequest.h"
#include "HomeKeyPolicy.h"
#include "HomeKeyCodeRequest.h"
#include <cassert>
#include <iostream>

int main()
{
    const uint8_t mac[6] = {0x24, 0x6f, 0x28, 0xab, 0xcd, 0xef};
    const auto setupId = HomeKey::pairingSetupId(mac);
    uint64_t macValue = 0;
    for (auto byte : mac) macValue = macValue * 256 + byte;
    uint32_t idValue = 0;
    for (size_t i = 0; i < 4; ++i)
    {
        const char digit = setupId[i];
        assert((digit >= '0' && digit <= '9') || (digit >= 'A' && digit <= 'Z'));
        idValue = idValue * 36 + (digit <= '9' ? digit - '0' : digit - 'A' + 10);
    }
    assert(idValue == macValue % 1679616 && setupId[4] == '\0');
    uint8_t otherMac[6] = {0x24, 0x6f, 0x28, 0xab, 0xcd, 0xf0};
    assert(HomeKey::pairingSetupId(otherMac) != setupId);
    assert(HomeKey::pairingSetupId(mac) == setupId);
    OpenKNX::Network::WebRequest request;
    request.setHeader("Content-Type", "text/plain");
    assert(request.header("Content-Type") == "text/plain");
    assert(request.header("X-HomeKey-Token").empty());
    request.uri = "/homekey?xmode=bad&mode=good%20value";
    assert(request.getQueryParam("mode") == "good value");
    const uint8_t code[] = {'2', '4', '6', '8', '1', '3', '5', '7'};
    request.setBody(code, sizeof(code));
    assert(request.body() == code && request.bodyLength() == 8);
    assert(HomeKey::validPairingCode("24681357"));
    assert(HomeKey::validPairingCode("00001234"));
    for (const auto& invalid : {"", "2468135", "246813579", "abcdefgh", "2468 357",
                               "00000000", "11111111", "99999999", "12345678", "87654321"})
        assert(!HomeKey::validPairingCode(invalid));
    const std::string token(32, 'a');
    HomeKey::CodeRequest parsed;
    auto parse = [&](const std::string& body)
    {
        return HomeKey::parseCodeRequest(reinterpret_cast<const uint8_t*>(body.data()), body.size(), parsed);
    };
    const std::string first = token + "\n\n24681357";
    const std::string replacement = token + "\n24681357\n13572468";
    assert(first.size() == 42 && parse(first));
    assert(std::string(parsed.token) == token && std::string(parsed.previous).empty());
    assert(std::string(parsed.code) == "24681357");
    assert(replacement.size() == 50 && parse(replacement));
    assert(std::string(parsed.previous) == "24681357" && std::string(parsed.code) == "13572468");
    assert(!HomeKey::parseCodeRequest(nullptr, 42, parsed));
    for (size_t length = 0; length < replacement.size(); ++length)
        assert(!HomeKey::parseCodeRequest(reinterpret_cast<const uint8_t*>(replacement.data()), length, parsed));
    for (const auto& invalid : {replacement + "\n", token + "\n2468135\n13572468",
                                token + "\n2468135x\n13572468", token + "\n\n12345678",
                                token + "\r\n\n24681357", std::string(32, 'g') + "\n\n24681357"})
        assert(!parse(invalid));
    auto embeddedNul = replacement;
    embeddedNul[38] = '\0';
    assert(!parse(embeddedNul));
    assert(parsed.code[0] == 0 && parsed.previous[0] == 0);

    const char empty[9] = {}, stored[9] = "24681357", wrong[9] = "13572468";
    assert(HomeKey::matchesPreviousCode(empty, empty));
    assert(!HomeKey::matchesPreviousCode(empty, stored));
    assert(!HomeKey::matchesPreviousCode(stored, empty));
    assert(!HomeKey::matchesPreviousCode(stored, wrong));
    assert(HomeKey::matchesPreviousCode(stored, stored));
    for (size_t i = 0; i < 8; ++i)
    {
        char changed[9];
        std::copy(std::begin(stored), std::end(stored), changed);
        changed[i] = changed[i] == '9' ? '0' : changed[i] + 1;
        assert(!HomeKey::matchesPreviousCode(stored, changed));
    }

    HomeKey::CodeAttemptLimit limit;
    for (unsigned i = 0; i < 4; ++i) { limit.failed(100); assert(limit.remaining(100) == 0); }
    limit.failed(100);
    assert(limit.failures == 5 && limit.remaining(100) == 300000);
    assert(limit.remaining(300099) == 1 && limit.remaining(300100) == 0);
    limit.restore(5, 500);
    assert(limit.remaining(500) == 300000); // A reboot restarts an exhausted cooldown.
    limit.clear();
    assert(limit.failures == 0 && limit.remaining(500) == 0);
    limit.restore(3, 1000);
    limit.failed(1000);
    assert(limit.failures == 4 && limit.remaining(1000) == 0);
    limit.failed(1000);
    assert(limit.remaining(1000) == 300000);
    limit.restore(5, UINT32_MAX - 1000);
    assert(limit.remaining(UINT32_MAX - 1000) == 300000);
    assert(limit.remaining(298999) == 0);
    std::cout << "Bounded request parsing, old-code authentication and persistent cooldown policy checks passed\n";
}
