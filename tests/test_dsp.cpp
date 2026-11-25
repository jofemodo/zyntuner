// Drives the built LV2 binary through its ports. See tests/run.sh
#include <lv2/core/lv2.h>
#include <lv2/urid/urid.h>
#include <lv2/options/options.h>
#include <lv2/buf-size/buf-size.h>
#include <lv2/atom/atom.h>
#include <lv2/parameters/parameters.h>
#include <dlfcn.h>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>
static std::map<std::string, LV2_URID> ids;
static LV2_URID mapUri(LV2_URID_Map_Handle, const char* u) { auto& i = ids[u]; if (!i) i = ids.size(); return i; }
static int fails = 0;
static void check(bool ok, const char* what) { if (!ok) ++fails; printf("%s %s\n", ok ? "ok  " : "FAIL", what); }
struct Host {
    const LV2_Descriptor* d; LV2_Handle h; double sr; int bs;
    float ref = 440, thr = -60, en = 1, note = 0, cents = 0, valid = 0;
    std::vector<float> l, r, ol, orr; double worst = 0, total = 0;
    Host(const LV2_Descriptor* d, double sr, int bs) : d(d), sr(sr), bs(bs), l(bs), r(bs), ol(bs), orr(bs) {
        static LV2_URID_Map m{ nullptr, mapUri };
        static int32_t len; len = bs; static float srf; srf = sr;
        LV2_URID ti = mapUri(0, LV2_ATOM__Int), tf = mapUri(0, LV2_ATOM__Float);
        LV2_Options_Option o[] = {
            { LV2_OPTIONS_INSTANCE, 0, mapUri(0, LV2_BUF_SIZE__maxBlockLength), 4, ti, &len },
            { LV2_OPTIONS_INSTANCE, 0, mapUri(0, LV2_BUF_SIZE__nominalBlockLength), 4, ti, &len },
            { LV2_OPTIONS_INSTANCE, 0, mapUri(0, LV2_PARAMETERS__sampleRate), 4, tf, &srf },
            { LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, nullptr } };
        LV2_Feature fm{ LV2_URID__map, &m }, fo{ LV2_OPTIONS__options, o };
        const LV2_Feature* f[] = { &fm, &fo, nullptr };
        h = d->instantiate(d, sr, "", f);
        if (!h) { puts("instantiate failed"); exit(2); }
        void* p[] = { l.data(), r.data(), ol.data(), orr.data(), &ref, &thr, &en, &note, &cents, &valid };
        for (int i = 0; i < 10; ++i) d->connect_port(h, i, p[i]);
        d->activate(h);
    }
    ~Host() { d->deactivate(h); d->cleanup(h); }
    bool same = true;
    template<class F> void run(double secs, F gen) {
        for (long n = 0; n < secs * sr; n += bs) {
            for (int i = 0; i < bs; ++i) { l[i] = gen((n + i) / sr); r[i] = -0.3f * l[i] + 0.0f; }
            auto t = std::chrono::steady_clock::now(); d->run(h, bs);
            double e = std::chrono::duration<double>(std::chrono::steady_clock::now() - t).count();
            worst = std::max(worst, e); total += e;
            for (int i = 0; i < bs; ++i) same &= ol[i] == l[i] && orr[i] == r[i];
        }
    }
};
static double hz(int n, double c) { return 440 * pow(2, (n - 69 + c / 100) / 12); }
static float noise() { return rand() / (float)RAND_MAX * 2 - 1; }
int main(int, char** a) {
    void* so = dlopen(a[1], RTLD_NOW); if (!so) { puts(dlerror()); return 2; }
    auto d = ((LV2_Descriptor_Function)dlsym(so, "lv2_descriptor"))(0);
    char b[200];
    for (double sr : { 44100., 48000., 96000. }) for (int bs : { 64, 256, 1024 }) {
        double maxErr = 0; int wrong = 0;
        for (int n : { 23, 28, 33, 38, 40, 45, 50, 55, 59, 64, 69, 76, 82, 88 }) for (double c : { -37., -8., 0., 3., 21. }) {
            Host h(d, sr, bs); double f = hz(n, c);
            // Plucked string: 10 partials, faster decay higher up, weak fundamental, some noise
            h.run(1.0, [&](double t) { double s = 0; for (int k = 1; k <= 10 && k * f < sr / 2; ++k)
                s += (k == 1 ? .5 : 1. / k) * exp(-t * (.6 + .5 * k)) * sin(2 * M_PI * k * f * t + k); return float(.2 * s + .001 * noise()); });
            if (h.valid < .5 || h.note != n) { ++wrong; if (sr == 48000) printf("  wrong n=%d c=%g got %g %g v%g\n", n, c, h.note, h.cents, h.valid); }
            else maxErr = std::max(maxErr, fabs(h.cents - c));
            if (!h.same) check(false, "passthrough");
        }
        snprintf(b, 200, "%g Hz / %d: wrong %d of 70, max error %.2f cents", sr, bs, wrong, maxErr); check(!wrong && maxErr < 2, b);
    }
    auto sine = [](double t) { return float(.2 * sin(2 * M_PI * 110 * t)); };
    { Host h(d, 48000, 256); h.run(1, sine); check(h.valid > .5 && h.note == 45, "A2 sine");
      h.run(1, [](double) { return 0.f; }); check(h.valid < .5, "silence clears valid");
      h.run(1, sine); h.en = 0; h.run(.1, sine); check(h.valid < .5 && h.same, "bypass: no pitch, audio passes");
      h.en = 1; h.run(1, sine); check(h.valid > .5, "un-bypass recovers");
      h.ref = 432; h.run(.5, sine); check(fabs(h.cents - 31.77) < 1, "reference 432"); }
    { Host h(d, 48000, 256); int v = 0; for (int i = 0; i < 50; ++i) { h.run(.1, [](double) { return .2f * noise(); }); v += h.valid > .5; }
      snprintf(b, 200, "white noise: valid %d of 50", v); check(v < 5, b); }
    { Host h(d, 48000, 64); h.run(20, sine); printf("CPU active %.3f %% of realtime, worst 64-frame block %.0f us\n", h.total / 20 * 100, h.worst * 1e6); }
    { Host h(d, 48000, 64); h.run(20, [](double) { return 0.f; }); printf("CPU silent %.3f %%\n", h.total / 20 * 100); }
    return fails != 0;
}
