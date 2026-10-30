#include "browser/browser_site.h"
#include "ui/browser_windows.h"
#include "ui/window_manager_state.h"
#include "render/renderer.h"
#include <assert.h>
#include <docobj.h>
#include <mshtmhst.h>
#include <ocidl.h>
#include <olectl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct UIWindowMgr g_windowMgr;
struct Renderer *g_renderer;
static int failure, live_blocks, allocations, releases, rect_queries;
static int fail_resize_query, fail_rects, fail_unadvise, fail_detach;
static int object_refs, container_refs, point_refs, inplace_refs, closes, unadvises;
static IOleClientSite *control_client;
static IDispatch *control_sink;
static IWebBrowser2 control;
static IOleObject object;
static IConnectionPointContainer container;
static IConnectionPoint point;
static IOleInPlaceObject inplace;
static RECT last_rect;

struct Block { size_t size; unsigned magic; };
static void *site_calloc(size_t count, size_t size)
{
    struct Block *block;
    if (failure == 1) return NULL;
    block = calloc(1, sizeof *block + count * size + 16);
    assert(block);
    block->size = count * size; block->magic = 0xc04a51e;
    memset((char *)(block + 1) + block->size, 0xa7, 16);
    ++live_blocks; ++allocations;
    return block + 1;
}
static void site_free(void *memory)
{
    struct Block *block;
    int i;
    if (!memory) return;
    block = (struct Block *)memory - 1;
    assert(block->magic == 0xc04a51e && live_blocks > 0);
    for (i = 0; i < 16; ++i) assert(((unsigned char *)memory)[block->size + i] == 0xa7);
    block->magic = 0; --live_blocks; ++releases; free(block);
}
static BOOL host_rect(HWND hwnd, RECT *rect)
{
    assert(hwnd == (HWND)1);
    ++rect_queries;
    if (failure == 5) return FALSE;
    SetRect(rect, 0, 0, 640, 480);
    return TRUE;
}
static HRESULT contained(IUnknown *unknown, BOOL is_contained)
{
    assert(unknown == (IUnknown *)&object && is_contained);
    return failure == 4 ? E_FAIL : S_OK;
}

#define calloc site_calloc
#define free site_free
#define GetClientRect host_rect
#define OleSetContainedObject contained
#include "../src/browser/web_sites.c"
#undef calloc
#undef free
#undef GetClientRect
#undef OleSetContainedObject

struct WebBrowser *ui_window_mgr_new_web_browser(struct UIWindowMgr *mgr, int width, int height)
{ (void)mgr; (void)width; (void)height; abort(); }
int web_browser_navigate(struct WebBrowser *browser, const char *url)
{ (void)browser; (void)url; abort(); }

static HRESULT STDMETHODCALLTYPE control_query(IWebBrowser2 *self, REFIID iid, void **out)
{
    assert(self == &control); *out = NULL;
    if (IsEqualIID(iid, &IID_IOleObject)) {
        if (failure == 2) return E_NOINTERFACE;
        *out = &object; ++object_refs; return S_OK;
    }
    if (IsEqualIID(iid, &IID_IOleInPlaceObject)) {
        if (fail_resize_query) return E_NOINTERFACE;
        *out = &inplace; ++inplace_refs; return S_OK;
    }
    return E_NOINTERFACE;
}
static HRESULT STDMETHODCALLTYPE object_query(IOleObject *self, REFIID iid, void **out)
{
    assert(self == &object); *out = NULL;
    if (!IsEqualIID(iid, &IID_IConnectionPointContainer) || failure == 7) return E_NOINTERFACE;
    *out = &container; ++container_refs; return S_OK;
}
static ULONG STDMETHODCALLTYPE object_release(IOleObject *self)
{ assert(self == &object && object_refs > 0); return --object_refs; }
static HRESULT STDMETHODCALLTYPE set_client(IOleObject *self, IOleClientSite *client)
{
    assert(self == &object);
    if (client && failure == 3) return E_FAIL;
    if (!client && fail_detach) return E_FAIL;
    if (client) client->lpVtbl->AddRef(client);
    if (control_client) control_client->lpVtbl->Release(control_client);
    control_client = client;
    return S_OK;
}
static HRESULT STDMETHODCALLTYPE do_verb(IOleObject *self, LONG verb, MSG *msg, IOleClientSite *client,
                                        LONG index, HWND hwnd, LPCRECT rect)
{
    IOleInPlaceSite *site;
    IOleInPlaceFrame *frame = NULL;
    IOleInPlaceUIWindow *doc = (IOleInPlaceUIWindow *)1;
    OLEINPLACEFRAMEINFO info;
    RECT pos, clip;
    HWND site_window;
    assert(self == &object && verb == OLEIVERB_INPLACEACTIVATE && !msg && !index);
    assert(client == control_client && hwnd == (HWND)1 && rect->right == 640 && rect->bottom == 480);
    assert(client->lpVtbl->QueryInterface(client, &IID_IOleInPlaceSite, (void **)&site) == S_OK);
    memset(&info, 0xa5, sizeof info); memset(&pos, 0xa5, sizeof pos); memset(&clip, 0xa5, sizeof clip);
    assert(site->lpVtbl->GetWindow(site, &site_window) == S_OK && site_window == hwnd);
    assert(site->lpVtbl->GetWindowContext(site, &frame, &doc, &pos, &clip, &info) == S_OK);
    assert(frame && !doc && EqualRect(&pos, rect) && EqualRect(&clip, rect));
    assert(info.cb == sizeof info && !info.fMDIApp && info.hwndFrame == hwnd && !info.haccel && !info.cAccelEntries);
    frame->lpVtbl->Release(frame); site->lpVtbl->Release(site);
    return failure == 6 ? E_FAIL : S_OK;
}
static HRESULT STDMETHODCALLTYPE close_object(IOleObject *self, DWORD save)
{
    IOleInPlaceSite *site;
    HWND hwnd = (HWND)99;
    RECT rect = {0, 0, 100, 100};
    assert(self == &object && save == OLECLOSE_NOSAVE); ++closes;
    if (control_client) {
        assert(control_client->lpVtbl->QueryInterface(control_client, &IID_IOleInPlaceSite, (void **)&site) == S_OK);
        assert(site->lpVtbl->GetWindow(site, &hwnd) == E_UNEXPECTED && !hwnd);
        assert(site->lpVtbl->OnPosRectChange(site, &rect) == E_UNEXPECTED);
        site->lpVtbl->Release(site);
    }
    return S_OK;
}
static ULONG STDMETHODCALLTYPE container_release(IConnectionPointContainer *self)
{ assert(self == &container && container_refs > 0); return --container_refs; }
static HRESULT STDMETHODCALLTYPE find_point(IConnectionPointContainer *self, REFIID iid, IConnectionPoint **out)
{
    assert(self == &container && IsEqualIID(iid, &DIID_DWebBrowserEvents2)); *out = NULL;
    if (failure == 8) return CONNECT_E_NOCONNECTION;
    *out = &point; ++point_refs; return S_OK;
}
static ULONG STDMETHODCALLTYPE point_release(IConnectionPoint *self)
{ assert(self == &point && point_refs > 0); return --point_refs; }
static HRESULT STDMETHODCALLTYPE advise(IConnectionPoint *self, IUnknown *sink, DWORD *cookie)
{
    IUnknown *identity;
    assert(self == &point && !control_sink);
    if (failure == 9) return CONNECT_E_CANNOTCONNECT;
    assert(sink->lpVtbl->QueryInterface(sink, &DIID_DWebBrowserEvents2, (void **)&control_sink) == S_OK);
    assert(control_sink->lpVtbl->QueryInterface(control_sink, &IID_IUnknown, (void **)&identity) == S_OK);
    assert(identity == (IUnknown *)control_client); identity->lpVtbl->Release(identity);
    *cookie = 42; return S_OK;
}
static HRESULT STDMETHODCALLTYPE unadvise(IConnectionPoint *self, DWORD cookie)
{
    assert(self == &point && cookie == 42 && control_sink); ++unadvises;
    if (fail_unadvise) return E_FAIL;
    control_sink->lpVtbl->Release(control_sink); control_sink = NULL;
    return S_OK;
}
static ULONG STDMETHODCALLTYPE inplace_release(IOleInPlaceObject *self)
{ assert(self == &inplace && inplace_refs > 0); return --inplace_refs; }
static HRESULT STDMETHODCALLTYPE set_rects(IOleInPlaceObject *self, LPCRECT pos, LPCRECT clip)
{
    assert(self == &inplace && EqualRect(pos, clip)); last_rect = *pos;
    return fail_rects ? E_FAIL : S_OK;
}

static IWebBrowser2Vtbl control_methods = {.QueryInterface = control_query};
static IOleObjectVtbl object_methods = {
    .QueryInterface = object_query, .Release = object_release, .SetClientSite = set_client,
    .DoVerb = do_verb, .Close = close_object
};
static IConnectionPointContainerVtbl container_methods = {.Release = container_release, .FindConnectionPoint = find_point};
static IConnectionPointVtbl point_methods = {.Release = point_release, .Advise = advise, .Unadvise = unadvise};
static IOleInPlaceObjectVtbl inplace_methods = {.Release = inplace_release, .SetObjectRects = set_rects};

static void check_balanced(void)
{
    assert(!object_refs && !container_refs && !point_refs && !inplace_refs);
    assert(!control_client && !control_sink && !live_blocks);
    assert(allocations == releases);
}

static void interfaces(void)
{
    const IID *ids[] = {&IID_IUnknown, &IID_IOleClientSite, &IID_IOleInPlaceSite, &IID_IOleWindow,
        &IID_IOleInPlaceFrame, &IID_IOleInPlaceUIWindow, &IID_IDocHostUIHandler, &IID_IDispatch,
        &DIID_DWebBrowserEvents2, &IID_IOleCommandTarget};
    IUnknown *from, *to, *identity;
    IOleInPlaceSite *in_place;
    IDocHostUIHandler *host;
    IOleCommandTarget *command;
    DOCHOSTUIINFO info;
    OLECMD cmd = {1, 0xa5a5};
    RECT rect = {1, 2, 320, 240};
    ITypeInfo *type_info = (ITypeInfo *)1;
    DISPID names[2] = {7, 8};
    UINT count = 99;
    unsigned a, b, round;
    for (round = 0; round < 50; ++round) for (a = 0; a < 10; ++a) {
        assert(control_client->lpVtbl->QueryInterface(control_client, ids[a], (void **)&from) == S_OK);
        for (b = 0; b < 10; ++b) {
            assert(from->lpVtbl->QueryInterface(from, ids[b], (void **)&to) == S_OK);
            assert(to->lpVtbl->QueryInterface(to, &IID_IUnknown, (void **)&identity) == S_OK);
            assert(identity == (IUnknown *)control_client);
            identity->lpVtbl->Release(identity); to->lpVtbl->Release(to);
        }
        from->lpVtbl->Release(from);
    }
    identity = (IUnknown *)1;
    assert(control_client->lpVtbl->QueryInterface(control_client, &IID_IStream, (void **)&identity) == E_NOINTERFACE && !identity);
    assert(control_client->lpVtbl->QueryInterface(control_client, &IID_IUnknown, NULL) == E_POINTER);
    assert(control_client->lpVtbl->QueryInterface(control_client, NULL, (void **)&identity) == E_POINTER && !identity);
    assert(control_client->lpVtbl->QueryInterface(control_client, &IID_IOleInPlaceSite, (void **)&in_place) == S_OK);
    assert(in_place->lpVtbl->OnPosRectChange(in_place, &rect) == S_OK && EqualRect(&last_rect, &rect) && !inplace_refs);
    fail_resize_query = 1;
    assert(in_place->lpVtbl->OnPosRectChange(in_place, &rect) == E_NOINTERFACE && !inplace_refs);
    fail_resize_query = 0; fail_rects = 1;
    assert(in_place->lpVtbl->OnPosRectChange(in_place, &rect) == E_FAIL && !inplace_refs);
    fail_rects = 0; in_place->lpVtbl->Release(in_place);
    assert(control_client->lpVtbl->QueryInterface(control_client, &IID_IDocHostUIHandler, (void **)&host) == S_OK);
    memset(&info, 0xa5, sizeof info);
    assert(host->lpVtbl->GetHostInfo(host, &info) == S_OK && info.cbSize == sizeof info);
    assert(info.dwFlags == DOCHOSTUIFLAG_NO3DBORDER && !info.pchHostCss && !info.pchHostNS);
    host->lpVtbl->Release(host);
    assert(control_sink->lpVtbl->GetTypeInfoCount(control_sink, &count) == S_OK && !count);
    assert(control_sink->lpVtbl->GetTypeInfo(control_sink, 0, 0, &type_info) == DISP_E_BADINDEX && !type_info);
    assert(control_sink->lpVtbl->GetIDsOfNames(control_sink, &IID_NULL, NULL, 2, 0, names) == DISP_E_UNKNOWNNAME);
    assert(names[0] == DISPID_UNKNOWN && names[1] == DISPID_UNKNOWN);
    assert(control_client->lpVtbl->QueryInterface(control_client, &IID_IOleCommandTarget, (void **)&command) == S_OK);
    assert(command->lpVtbl->QueryStatus(command, NULL, 1, &cmd, NULL) == OLECMDERR_E_NOTSUPPORTED && !cmd.cmdf);
    command->lpVtbl->Release(command);
}

int main(void)
{
    struct BrowserSite *site = (struct BrowserSite *)1;
    IOleInPlaceSite *retained;
    HWND hwnd;
    int i, queries;
    control.lpVtbl = &control_methods; object.lpVtbl = &object_methods;
    container.lpVtbl = &container_methods; point.lpVtbl = &point_methods; inplace.lpVtbl = &inplace_methods;
    assert(browser_site_open((HWND)1, &control, NULL) == E_POINTER);
    assert(browser_site_open(NULL, &control, &site) == E_INVALIDARG && !site);
    assert(browser_site_open((HWND)1, NULL, &site) == E_INVALIDARG && !site);
    for (failure = 1; failure <= 9; ++failure) {
        site = (struct BrowserSite *)1;
        assert(FAILED(browser_site_open((HWND)1, &control, &site)) && !site);
        check_balanced();
    }
    failure = 0;
    for (i = 0; i < 100; ++i) {
        assert(browser_site_open((HWND)1, &control, &site) == S_OK && site);
        if (!i) interfaces();
        assert(control_client->lpVtbl->QueryInterface(control_client, &IID_IOleInPlaceSite, (void **)&retained) == S_OK);
        queries = rect_queries;
        browser_site_close(site);
        assert(live_blocks == 1 && !control_client && !control_sink);
        hwnd = (HWND)1;
        assert(retained->lpVtbl->GetWindow(retained, &hwnd) == E_UNEXPECTED && !hwnd && rect_queries == queries);
        retained->lpVtbl->Release(retained);
        check_balanced();
    }
    assert(browser_site_open((HWND)1, &control, &site) == S_OK);
    fail_unadvise = fail_detach = 1;
    browser_site_close(site);
    assert(live_blocks == 1 && control_client && control_sink);
    {
        VARIANT args[5] = {{0}};
        VARIANT_BOOL cancel = VARIANT_FALSE;
        DISPPARAMS params = {args, NULL, 5, 0};
        args[0].vt = VT_BSTR;
        args[3].vt = VT_BYREF | VT_BOOL;
        args[3].pboolVal = &cancel;
        assert(control_sink->lpVtbl->Invoke(control_sink, DISPID_NEWWINDOW3, &IID_NULL, 0,
                                           DISPATCH_METHOD, &params, NULL, NULL, NULL) == S_OK);
        assert(cancel == VARIANT_TRUE);
    }
    control_sink->lpVtbl->Release(control_sink); control_sink = NULL;
    control_client->lpVtbl->Release(control_client); control_client = NULL;
    check_balanced();
    browser_site_close(NULL);
    assert(closes > 100 && unadvises == 101);
    puts("Browser COM: SDK signatures, shared identity, 5000 interface queries, nine setup failures, retained callbacks, resize failures, detached cleanup and failed disconnects passed");
    return 0;
}
