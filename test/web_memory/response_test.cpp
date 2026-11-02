#include "OpenKNX/Network/Webserver/WebResponse.h"
#include "OpenKNX/Network/Webserver/HeapAdmission.h"
#include "OpenKNX/Helper.h"
#include <cassert>
#include <iostream>
#include <string>

using namespace OpenKNX::Network;

static std::string readResponse(WebResponse& response, size_t step)
{
    uint8_t buffer[513];
    std::string result;
    for (size_t count; (count = response.readStreamChunk(buffer, step));)
        result.append(reinterpret_cast<char*>(buffer), count);
    return result;
}

int main()
{
    // A fragmented heap cannot provide the old 4/8-KiB growing page string.
    // Real response assembly must still work with allocations limited to 1 KiB.
    blockLimit = 1024;
    const std::string body(12000, 'x');
    for (size_t step : {size_t(1), size_t(17), size_t(512), size_t(513)})
    {
        WebResponse response;
        HtmlWriter html(response);
        html += "prefix";
        html += body;
        html += "suffix";
        response.setLayout(true);
        response.setLayoutChrome("<header>", "<footer>");
        assert(response.statusCode() == 200 && response.isStreaming());
        assert(response.streamTotal() == body.size() + 28);
        assert(readResponse(response, step) == "<header>prefix" + body + "suffix<footer>");
        response.cleanupStream();
        response.cleanupStream();
    }
    assert(largestAllocation <= 1024);

    // Fail context creation and each subsequent chunk allocation. Partial HTML
    // must be discarded, never returned with HTTP 200 or decorated with layout.
    for (size_t failure = 0; failure < 7; ++failure)
    {
        allocationCount = 0;
        failAt = failure;
        WebResponse response;
        HtmlWriter html(response);
        html += body;
        html += "still unavailable";
        response.setLayout(true);
        assert(response.statusCode() == 503 && !response.isStreaming() && !response.useLayout());
        response.finalizeSegments();
        assert(response.totalLength() > 0);
        assert(std::string(response.body()).find("insufficient memory") != std::string::npos);
        failAt = static_cast<size_t>(-1);
        response.reset();
        assert(response.statusCode() == 200);
        HtmlWriter retry(response);
        retry += "recovered";
        assert(readResponse(response, 17) == "recovered");
    }

    // A normal copied body must report an allocation failure as 503, too.
    WebResponse failure;
    failAt = 0; allocationCount = 0;
    failure.send("copy");
    assert(failure.statusCode() == 503 && failure.bodyLength() > 0);
    failAt = static_cast<size_t>(-1);

    // Keep RP2040's deferred file cleanup contract: reset must not close a file
    // from a network interrupt after its callback was parked for the loop task.
    unsigned cleaned = 0;
    WebResponse file;
    file.sendStream(0, [](void*, uint8_t*, size_t) { return size_t(0); },
        [](void* context) { ++*static_cast<unsigned*>(context); }, &cleaned);
    auto cleanup = file.streamCleanupFn();
    auto context = file.streamCtx();
    file.reset();
    assert(cleaned == 0);
    cleanup(context);
    assert(cleaned == 1);

    assert(webHeapFits(20000, 4096, 1024, 16384, 4096));
    assert(!webHeapFits(17000, 4096, 1024, 16384, 4096));
    assert(!webHeapFits(20000, 2048, 0, 16384, 4096));
    assert(!webHeapFits(20000, 4096, static_cast<size_t>(-1), 16384, 4096));
    std::cout << "Web response fragmentation, allocation failure and stream cleanup checks passed\n";
}
