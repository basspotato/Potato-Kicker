#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <iterator>

// Potato-Kicker synthesis engine.
// One oscillator with an exponential pitch envelope, an ADSR amp envelope, a noise/tone click,
// tanh drive, a peak compressor, output volume and an optional stereo widener that keeps the lows mono.
// The same code renders live audio and the offline preview drawn in the editor's display.
namespace pk
{
    struct EngineParams
    {
        int   wave = 0;
        float tune = 52.0f, pamt = 30.0f, pdec = 30.0f;
        float atk = 0.0f, dec = 240.0f, sus = 0.0f, rel = 40.0f;
        float click = 50.0f, drive = 25.0f;
        float thr = -14.0f, ratio = 5.0f, makeup = 4.0f, vol = -3.0f;
        bool  comp = true, stereo = false, keytrack = true, velocity = true;

        bool oneShot() const noexcept { return sus < 0.05f; }

        bool operator== (const EngineParams& o) const noexcept
        {
            return wave == o.wave && tune == o.tune && pamt == o.pamt && pdec == o.pdec && atk == o.atk && dec == o.dec
                && sus == o.sus && rel == o.rel && click == o.click && drive == o.drive && thr == o.thr && ratio == o.ratio
                && makeup == o.makeup && vol == o.vol && comp == o.comp && stereo == o.stereo && keytrack == o.keytrack && velocity == o.velocity;
        }

        // Amp envelope while the note is held (or for the whole note in one-shot mode)
        float envHold (float t) const noexcept
        {
            const float A = atk * 0.001f, D = std::max (0.001f, dec * 0.001f), S = sus * 0.01f;
            if (t < A) { const float x = 1.0f - t / A; return 1.0f - x * x; }
            const float x = (t - A) / D;
            if (x >= 1.0f) return S;
            static const float e5 = std::exp (-5.0f);
            return S + (1.0f - S) * (std::exp (-5.0f * x) - e5) / (1.0f - e5);
        }

        float pitchAt (float t) const noexcept
        {
            const float tau = std::max (0.0005f, pdec / 3000.0f);
            return tune * std::exp2 ((pamt / 12.0f) * std::exp (-t / tau));
        }
    };

    class KickEngine
    {
    public:
        static constexpr int maxVoices = 4;

        void prepare (double sampleRate, uint32_t seed = 0x1234567u)
        {
            sr = sampleRate;
            rng = seed ? seed : 1u;
            for (auto& v : voices) v = Voice {};
            cEnv = sx = sy = 0.0f;
            std::fill (std::begin (delay), std::end (delay), 0.0f);
            di = 0;
            delayLen = std::clamp ((int) std::lround (0.00025 * sr), 1, delaySize - 1);
            clickA  = (float) std::exp (-2.0 * 3.14159265358979 * 1400.0 / sr);
            sideA   = (float) std::exp (-2.0 * 3.14159265358979 * 300.0 / sr);
            attC    = (float) std::exp (-1.0 / (0.001 * sr));
            relC    = (float) std::exp (-1.0 / (0.12 * sr));
            grMax = 0.0f;
        }

        void setParams (const EngineParams& p) noexcept { prm = p; }
        const EngineParams& params() const noexcept { return prm; }

        void noteOn (int note, int velocity) noexcept
        {
            ++counter;
            for (auto& v : voices) if (v.active) v.fading = true;

            Voice* slot = nullptr;
            for (auto& v : voices) if (! v.active) { slot = &v; break; }
            if (slot == nullptr)
            {
                slot = &voices[0];
                for (auto& v : voices) if (v.age < slot->age) slot = &v;
            }
            *slot = Voice {};
            slot->active = true;
            slot->note = note;
            slot->age = counter;
            slot->vel = prm.velocity ? (float) velocity / 127.0f : 1.0f;
            slot->ratio = prm.keytrack ? std::exp2 ((float) (note - 36) / 12.0f) : 1.0f;
            // fixed start phases for the detuned saws so every hit sounds the same
            static const float startPh[6] { 0.13f, 0.71f, 0.42f, 0.88f, 0.27f, 0.59f };
            std::copy (std::begin (startPh), std::end (startPh), slot->ph2);
        }

        void noteOff (int note) noexcept
        {
            if (prm.oneShot()) return;   // one-shot kicks always play their full decay
            for (auto& v : voices)
                if (v.active && v.note == note && v.gate && ! v.fading)
                { v.gate = false; v.rt = v.t; v.rl = v.lv; }
        }

        void allNotesOff() noexcept
        {
            for (auto& v : voices) if (v.active) v.fading = true;
        }

        // Renders numSamples into L/R (R may be null for a mono bus). Overwrites the buffers.
        void render (float* L, float* R, int numSamples, float& sumSquares, float& grPeak) noexcept
        {
            const float dt = (float) (1.0 / sr);
            const float tau = std::max (0.0005f, prm.pdec / 3000.0f);
            const float oct = prm.pamt / 12.0f;
            const float ck = prm.click * 0.01f;
            const float k = 0.001f + prm.drive * 0.01f * 12.0f, tk = std::tanh (k);
            const float Rr = std::max (0.005f, prm.rel * 0.001f);
            const float AD = (prm.atk + prm.dec) * 0.001f;
            const bool oneShot = prm.oneShot();
            const float ir = 1.0f - 1.0f / prm.ratio;
            const float vol = std::pow (10.0f, prm.vol / 20.0f);
            const float fadeStep = dt / 0.004f;

            for (int i = 0; i < numSamples; ++i)
            {
                float x = 0.0f;
                for (auto& v : voices)
                {
                    if (! v.active) continue;
                    float e;
                    if (v.gate)
                    {
                        e = prm.envHold (v.t);
                        if (oneShot && v.t > AD) { v.active = false; continue; }
                    }
                    else
                    {
                        const float r = (v.t - v.rt) / Rr;
                        if (r >= 1.0f) { v.active = false; continue; }
                        e = v.rl * (1.0f - r) * (1.0f - r);
                    }
                    v.lv = e;
                    if (v.fading)
                    {
                        v.fade -= fadeStep;
                        if (v.fade <= 0.0f) { v.active = false; continue; }
                    }

                    const float f = prm.tune * v.ratio * std::exp2 (oct * std::exp (-v.t / tau));
                    const float pd = std::min (0.45f, f * dt);
                    float s = voiceOsc (v, prm.wave, pd, e) * e * 0.9f;
                    v.ph += pd;
                    if (v.ph >= 1.0f) v.ph -= 1.0f;

                    if (ck > 0.0f && v.t < 0.04f)
                    {
                        const float nz = nextRandom() * 2.0f - 1.0f;
                        const float y = clickA * (v.hy + nz - v.hx);
                        v.hx = nz; v.hy = y;
                        s += ck * (y * 0.9f * std::exp (-v.t / 0.0025f)
                                   + 0.6f * std::sin (18849.556f * v.t) * std::exp (-v.t / 0.0012f));
                    }
                    x += s * v.vel * v.fade;
                    v.t += dt;
                }

                // drive
                x = std::tanh (k * x) / tk;

                // compressor
                if (prm.comp)
                {
                    const float a = std::abs (x);
                    cEnv = a > cEnv ? attC * cEnv + (1.0f - attC) * a : relC * cEnv + (1.0f - relC) * a;
                    const float over = 20.0f * std::log10 (cEnv + 1.0e-9f) - prm.thr;
                    const float gr = over > 0.0f ? over * ir : 0.0f;
                    grPeak = std::max (grPeak, gr);
                    x *= std::pow (10.0f, (prm.makeup - gr) / 20.0f);
                }

                x *= vol;

                // stereo widener: high-passed, slightly delayed side signal, lows stay mono
                sy = sideA * (sy + x - sx); sx = x;
                const float d = delay[(di - delayLen + delaySize) % delaySize];
                delay[di] = sy; di = (di + 1) % delaySize;

                float l = x, r = x;
                if (prm.stereo) { l = x + 0.45f * d; r = x - 0.45f * d; }
                L[i] = softCeiling (l);
                if (R != nullptr) R[i] = softCeiling (r);
                sumSquares += x * x;
            }
            grMax = std::max (grMax, grPeak);
        }

        float maxGainReduction() const noexcept { return grMax; }

    private:
        struct Voice
        {
            bool active = false, gate = true, fading = false;
            int note = 36;
            uint32_t age = 0;
            float ph2[6] {};
            float vel = 1.0f, ratio = 1.0f, t = 0.0f, ph = 0.0f, rt = 0.0f, rl = 0.0f, lv = 1.0f, fade = 1.0f, hx = 0.0f, hy = 0.0f;
        };

        // Transparent below 0.8, rounds peaks off smoothly so the output never exceeds 1.0
        static float softCeiling (float x) noexcept
        {
            const float a = std::abs (x);
            if (a <= 0.8f) return x;
            return std::copysign (0.8f + 0.2f * std::tanh ((a - 0.8f) / 0.2f), x);
        }

        static float saw (float p, float dt) noexcept
        {
            float q = p + 0.5f; if (q >= 1.0f) q -= 1.0f;
            return 2.0f * q - 1.0f - blep (q, dt);
        }

        // Oscillator for one voice. 0-3: sine, triangle, saw, square. 4: supersaw (7 detuned saws),
        // 5: reese (2 detuned saws), 6: FM (sine with a 2:1 modulator whose depth follows the amp envelope)
        static float voiceOsc (Voice& v, int w, float dt, float env) noexcept
        {
            if (w < 4) return osc (w, v.ph, dt);
            if (w == 6)
            {
                float m = v.ph * 2.0f; m -= std::floor (m);
                const float index = 0.6f + 2.4f * env;   // brighter on the attack, rounder in the tail
                return std::sin (6.2831853f * v.ph + index * std::sin (6.2831853f * m));
            }
            auto advance = [] (float& p, float inc) { p += inc; if (p >= 1.0f) p -= 1.0f; };
            if (w == 5)
            {
                const float inc = dt * 1.0075f;
                const float y = 0.55f * (saw (v.ph, dt) + saw (v.ph2[0], inc));
                advance (v.ph2[0], inc);
                return y;
            }
            static const float detune[6] { -0.0110f, -0.0064f, -0.0020f, 0.0021f, 0.0066f, 0.0108f };
            float sides = 0.0f;
            for (int k = 0; k < 6; ++k)
            {
                const float inc = dt * (1.0f + detune[k]);
                sides += saw (v.ph2[k], inc);
                advance (v.ph2[k], inc);
            }
            return 0.5f * saw (v.ph, dt) + 0.2f * sides;
        }

        static float blep (float t, float dt) noexcept
        {
            if (t < dt)        { t /= dt; return t + t - t * t - 1.0f; }
            if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
            return 0.0f;
        }

        static float osc (int w, float p, float dt) noexcept
        {
            switch (w)
            {
                case 0:  return std::sin (6.2831853f * p);
                case 1:  return p < 0.25f ? 4.0f * p : (p < 0.75f ? 2.0f - 4.0f * p : 4.0f * p - 4.0f);
                case 2:  { float q = p + 0.5f; if (q >= 1.0f) q -= 1.0f; return (2.0f * q - 1.0f - blep (q, dt)) * 0.85f; }
                default:
                {
                    float y = p < 0.5f ? 1.0f : -1.0f;
                    float q = p + 0.5f; if (q >= 1.0f) q -= 1.0f;
                    y += blep (p, dt);
                    y -= blep (q, dt);
                    return y * 0.75f;
                }
            }
        }

        float nextRandom() noexcept
        {
            rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
            return (float) (rng & 0xffffff) / 16777216.0f;
        }

        static constexpr int delaySize = 64;
        double sr = 48000.0;
        EngineParams prm;
        Voice voices[maxVoices];
        uint32_t counter = 0, rng = 1;
        float cEnv = 0.0f, sx = 0.0f, sy = 0.0f;
        float delay[delaySize] {};
        int di = 0, delayLen = 12;
        float clickA = 0.82f, sideA = 0.96f, attC = 0.0f, relC = 0.0f, grMax = 0.0f;
    };
}
