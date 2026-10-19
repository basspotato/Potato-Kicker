#include "PluginEditor.h"
#include <BinaryData.h>

using namespace juce;

//==============================================================================
// Palette and fonts
namespace col
{
    const Colour page    { 0xff0a0a0b };
    const Colour ink     { 0xffe0dcd3 };
    const Colour inkDim  { 0xff8f8b84 };
    const Colour amber   { 0xffffb547 };
    const Colour amberHi { 0xffffd58a };
    const Colour led     { 0xffff2b1e };
    const Colour ledOff  { 0xff3a0e0b };
    const Colour go      { 0xff36d46d };
    const Colour cream   { 0xfff7e6bd };
    const Colour tick    { 0xffd2cdc3 };
    const Colour faceInk { 0xff2a1d0c };
    const Colour faceRed { 0xffc4241a };
}

namespace
{
    struct Typefaces
    {
        Typeface::Ptr semi = Typeface::createSystemTypefaceFor (BinaryData::BarlowCondensedSemiBold_ttf, BinaryData::BarlowCondensedSemiBold_ttfSize);
        Typeface::Ptr bold = Typeface::createSystemTypefaceFor (BinaryData::BarlowCondensedBold_ttf, BinaryData::BarlowCondensedBold_ttfSize);
        Typeface::Ptr mono = Typeface::createSystemTypefaceFor (BinaryData::ShareTechMonoRegular_ttf, BinaryData::ShareTechMonoRegular_ttfSize);
    };
    const Typefaces& tf() { static Typefaces t; return t; }

    Font label (float h, float kern = 0.0f, bool bold = false)
    {
        return Font (FontOptions().withTypeface (bold ? tf().bold : tf().semi).withHeight (h).withKerningFactor (kern));
    }
    Font mono (float h) { return Font (FontOptions().withTypeface (tf().mono).withHeight (h)); }

    float degToRad (float d) { return d * MathConstants<float>::pi / 180.0f; }
    Point<float> polar (Point<float> c, float r, float deg) { return { c.x + std::sin (degToRad (deg)) * r, c.y - std::cos (degToRad (deg)) * r }; }

    void drawLed (Graphics& g, Rectangle<float> r, bool on)
    {
        const auto c = r.getCentre();
        if (on)
        {
            g.setGradientFill (ColourGradient (col::led.withAlpha (0.45f), c.x, c.y, col::led.withAlpha (0.0f), c.x + r.getWidth() * 1.4f, c.y, true));
            g.fillEllipse (r.expanded (r.getWidth() * 0.9f));
            ColourGradient gr (Colour (0xffffd2c8), c.x - r.getWidth() * 0.12f, c.y - r.getHeight() * 0.15f, Colour (0xffa3120b), c.x + r.getWidth() * 0.5f, c.y, true);
            gr.addColour (0.45, col::led);
            g.setGradientFill (gr);
        }
        else
        {
            g.setGradientFill (ColourGradient (Colour (0xff5a1712), c.x - r.getWidth() * 0.1f, c.y - r.getHeight() * 0.15f, col::ledOff, c.x + r.getWidth() * 0.5f, c.y, true));
        }
        g.fillEllipse (r);
        g.setColour (Colours::black);
        g.drawEllipse (r, 0.8f);
    }

    void glowText (Graphics& g, const String& s, Rectangle<float> r, Justification j, Colour c, float glow)
    {
        g.setColour (c.withAlpha (glow));
        for (auto d : { Point<float> (-1, 0), Point<float> (1, 0), Point<float> (0, -1), Point<float> (0, 1) })
            g.drawText (s, r.translated (d.x, d.y), j, false);
        g.setColour (c);
        g.drawText (s, r, j, false);
    }

    void insetWindow (Graphics& g, Rectangle<float> r)
    {
        g.setColour (Colour (0xff2a2a2e)); g.fillRoundedRectangle (r.expanded (1.0f), 4.0f);
        g.setColour (Colour (0xff050404)); g.fillRoundedRectangle (r, 3.0f);
        g.setGradientFill (ColourGradient (Colours::black.withAlpha (0.8f), r.getX(), r.getY(), Colours::transparentBlack, r.getX(), r.getY() + 7.0f, false));
        g.fillRoundedRectangle (r, 3.0f);
    }

    // Black glossy knob with tick ring (and optional 0..10 numbers), drawn like the hardware reference
    void drawKnob (Graphics& g, Point<float> c, float size, float norm, bool numbered, bool ticks = true)
    {
        const float rO = size * 0.5f - 1.5f, rB = size * 0.3f;
        if (ticks)
        {
            for (int i = 0; i <= 40; ++i)
            {
                const bool major = i % 4 == 0;
                const float a = -135.0f + i * 6.75f;
                g.setColour (col::tick.withAlpha (major ? 0.95f : 0.55f));
                const auto p1 = polar (c, major ? rO - 5.0f : rO - 3.0f, a), p2 = polar (c, rO, a);
                g.drawLine (p1.x, p1.y, p2.x, p2.y, major ? 1.3f : 0.7f);
            }
        }
        if (numbered)
        {
            const float rn = rB + (rO - 5.0f - rB) * 0.5f + 2.0f;
            g.setFont (label (size * 0.084f * 1.15f));
            g.setColour (Colour (0xffd8d3c9));
            for (int j = 0; j <= 10; ++j)
            {
                const auto p = polar (c, rn, -135.0f + j * 27.0f);
                g.drawText (String (j), Rectangle<float> (12.0f, 10.0f).withCentre (p), Justification::centred, false);
            }
        }
        // skirt
        g.setGradientFill (ColourGradient (Colour (0xff3c3c41), c.x, c.y - rB - 3, Colours::black, c.x, c.y + rB + 3, false));
        g.fillEllipse (Rectangle<float> ((rB + 2.6f) * 2, (rB + 2.6f) * 2).withCentre (c));
        // body
        ColourGradient body (Colour (0xff505056), c.x - rB * 0.2f, c.y - rB * 0.4f, Colour (0xff020202), c.x - rB * 0.2f + rB * 1.56f, c.y - rB * 0.4f, true);
        body.addColour (0.32, Colour (0xff1d1d20));
        g.setGradientFill (body);
        const auto br = Rectangle<float> (rB * 2, rB * 2).withCentre (c);
        g.fillEllipse (br);
        g.setColour (Colours::black); g.drawEllipse (br, 0.8f);
        // shine
        const Point<float> sc (c.x - rB * 0.22f, c.y - rB * 0.4f);
        g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.26f), sc.x, sc.y, Colours::white.withAlpha (0.0f), sc.x + rB * 0.55f, sc.y, true));
        g.fillEllipse (Rectangle<float> (rB * 1.1f, rB * 0.6f).withCentre (sc));
        // pointer
        const float a = -135.0f + 270.0f * norm;
        const auto p1 = polar (c, rB * 0.15f, a), p2 = polar (c, rB - 2.0f, a);
        g.setColour (Colour (0xfff5f1e8));
        g.drawLine (p1.x, p1.y, p2.x, p2.y, jmax (2.0f, size * 0.026f));
    }
    // Envelope knob: the black Shadow Hills style knob with a backlit light-pipe ring set into a groove.
    // The ring, the lit scale ticks and the under-glow get brighter and shift toward a lighter tint as the
    // knob turns right, and dim back down as it turns left.
    void drawLitKnob (Graphics& g, Point<float> c, float size, float norm, Colour base, Colour peak, bool numbered = true, bool ticks = true)
    {
        const float n = jlimit (0.0f, 1.0f, norm);
        const float k = 0.25f + 0.75f * n;                       // light intensity
        const Colour col = base.interpolatedWith (peak, n * 0.75f);
        const float rO = size * 0.5f - 1.5f;
        const float rB = size * 0.25f;                           // knob body (slightly smaller to make room for the ring)
        const float rw = size * 0.034f;                          // light-pipe width
        const float rR = rB + rw * 0.5f + size * 0.04f;          // light-pipe radius
        const float a0 = degToRad (-135.0f), a = degToRad (-135.0f + 270.0f * n);

        // scale ticks: lit in the accent colour up to the current value
        for (int i = 0; ticks && i <= 40; ++i)
        {
            const bool major = i % 4 == 0;
            const float t = (float) i / 40.0f;
            const bool on = t <= n + 0.0001f && n > 0.001f;
            const auto p1 = polar (c, major ? rO - 5.0f : rO - 3.0f, -135.0f + 270.0f * t), p2 = polar (c, rO, -135.0f + 270.0f * t);
            if (on)
            {
                g.setColour (col.withAlpha (0.25f * k));
                g.drawLine (p1.x, p1.y, p2.x, p2.y, major ? 3.4f : 2.4f);
            }
            g.setColour (on ? col.brighter (0.3f).withAlpha (0.55f + 0.45f * k) : col::tick.withAlpha (major ? 0.95f : 0.55f));
            g.drawLine (p1.x, p1.y, p2.x, p2.y, major ? 1.3f : 0.8f);
        }

        // numbers 0..10, pushed out to sit between the ticks and the ring
        const float rn = 0.5f * ((rR + rw * 0.5f + 1.2f) + (rO - 5.0f));
        g.setFont (label (size * 0.084f * 1.15f));
        for (int j = 0; numbered && j <= 10; ++j)
        {
            const bool on = j / 10.0f <= n + 0.0001f && n > 0.001f;
            g.setColour (on ? Colour (0xfff2eee6) : Colour (0xffb9b4aa));
            g.drawText (String (j), Rectangle<float> (12.0f, 10.0f).withCentre (polar (c, rn, -135.0f + j * 27.0f)), Justification::centred, false);
        }

        // recessed groove and the dim backlit ring
        const auto ringBox = [&] (float r) { return Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c); };
        g.setColour (Colour (0xff040405)); g.drawEllipse (ringBox (rR), rw + 3.0f);
        g.setColour (Colours::white.withAlpha (0.06f)); g.drawEllipse (ringBox (rR + rw * 0.5f + 1.6f), 0.8f);
        g.setColour (col.withAlpha (0.05f + 0.10f * k)); g.drawEllipse (ringBox (rR), rw * 2.4f);
        g.setColour (col.withAlpha (0.10f + 0.28f * k)); g.drawEllipse (ringBox (rR), rw * 0.8f);

        // value arc with bloom; every layer scales with the knob position
        if (n > 0.001f)
        {
            Path arc; arc.addCentredArc (c.x, c.y, rR, rR, 0.0f, a0, a, true);
            auto stroke = [&] (float wdt, Colour cc) { g.setColour (cc); g.strokePath (arc, PathStrokeType (wdt, PathStrokeType::curved, PathStrokeType::rounded)); };
            stroke (rw * 3.6f, col.withAlpha (0.16f * k));
            stroke (rw * 2.2f, col.withAlpha (0.30f * k));
            stroke (rw * 1.1f, col.withAlpha (0.55f + 0.40f * k));
            stroke (rw * 0.7f, col.brighter (0.25f + 0.35f * n));
            stroke (1.0f, Colours::white.withAlpha (0.25f + 0.55f * n));
        }

        // light spilling onto the skirt from the ring
        g.setColour (col.withAlpha (0.10f + 0.30f * k)); g.drawEllipse (ringBox (rB + 2.2f), 1.6f);

        // skirt, glossy body, shine (same finish as the other knobs)
        g.setGradientFill (ColourGradient (Colour (0xff3c3c41), c.x, c.y - rB - 3, Colours::black, c.x, c.y + rB + 3, false));
        g.fillEllipse (ringBox (rB + 1.6f));
        ColourGradient body (Colour (0xff505056), c.x - rB * 0.2f, c.y - rB * 0.4f, Colour (0xff020202), c.x - rB * 0.2f + rB * 1.56f, c.y - rB * 0.4f, true);
        body.addColour (0.32, Colour (0xff1d1d20));
        g.setGradientFill (body); g.fillEllipse (ringBox (rB - 1.0f));
        g.setColour (Colours::black); g.drawEllipse (ringBox (rB - 1.0f), 0.8f);
        const Point<float> sc (c.x - rB * 0.22f, c.y - rB * 0.4f);
        g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.26f), sc.x, sc.y, Colours::white.withAlpha (0.0f), sc.x + rB * 0.55f, sc.y, true));
        g.fillEllipse (Rectangle<float> (rB * 1.1f, rB * 0.6f).withCentre (sc));

        // pointer: white line over a soft glow in the accent colour
        const auto p1 = polar (c, rB * 0.15f, -135.0f + 270.0f * n), p2 = polar (c, rB - 3.0f, -135.0f + 270.0f * n);
        g.setColour (col.withAlpha (0.20f + 0.45f * k)); g.drawLine (p1.x, p1.y, p2.x, p2.y, 5.0f);
        g.setColour (Colour (0xfff5f1e8)); g.drawLine (p1.x, p1.y, p2.x, p2.y, jmax (2.0f, size * 0.026f));
        g.setColour (col.brighter (0.5f).withAlpha (0.4f + 0.6f * k)); g.fillEllipse (Rectangle<float> (3.6f, 3.6f).withCentre (p2));
    }
}

//==============================================================================
// Rotary knob bound to one parameter
class Knob : public Component
{
public:
    Knob (RangedAudioParameter& p, String title, float diameter)
        : param (p), name (std::move (title)), size (diameter),
          attach (p, [this] (float) { repaint(); }, nullptr)
    {
        attach.sendInitialUpdate();
        setWantsKeyboardFocus (true);
        setMouseCursor (MouseCursor::UpDownResizeCursor);
        setTitle (name);
    }

    void placeAt (int cx, int cy) { setBounds (cx - ((int) size + 40) / 2, cy - (int) size / 2 - 18, (int) size + 40, (int) size + 34); }

    // Gives this knob a backlit ring: colour at the left end of travel, and the lighter tint it moves toward at full right
    void setAccent (Colour base, Colour peak) { accent = base; accentPeak = peak; lit = true; repaint(); }

    void paint (Graphics& g) override
    {
        const float w = (float) getWidth();
        g.setColour (col::ink); g.setFont (label (13.5f, 0.12f));
        g.drawText (name, Rectangle<float> (0, 0, w, 17), Justification::centred, false);
        if (lit) drawLitKnob (g, { w * 0.5f, 18.0f + size * 0.5f }, size, param.getValue(), accent, accentPeak, size >= 84.0f);
        else     drawKnob (g, { w * 0.5f, 18.0f + size * 0.5f }, size, param.getValue(), size >= 84.0f);
        const Colour valueCol = lit ? accent.interpolatedWith (accentPeak, 0.6f) : col::amber;
        g.setColour (dragging ? valueCol : col::inkDim); g.setFont (mono (11.0f));
        g.drawText (pk::formatValue (param.getParameterID(), value()), Rectangle<float> (0, 18.0f + size, w, 15), Justification::centred, false);
        if (hasKeyboardFocus (false))
        {
            g.setColour (col::amber.withAlpha (0.6f));
            g.drawEllipse (Rectangle<float> (size * 0.66f, size * 0.66f).withCentre ({ w * 0.5f, 18.0f + size * 0.5f }), 1.5f);
        }
    }

    void mouseDown (const MouseEvent& e) override
    {
        grabKeyboardFocus();
        attach.beginGesture();
        dragging = true;
        startY = e.getScreenY();
        startN = param.getValue();
        repaint();
    }
    void mouseDrag (const MouseEvent& e) override
    {
        const float n = startN + (float) (startY - e.getScreenY()) / (e.mods.isShiftDown() ? 900.0f : 220.0f);
        attach.setValueAsPartOfGesture (param.convertFrom0to1 (jlimit (0.0f, 1.0f, n)));
    }
    void mouseUp (const MouseEvent&) override { attach.endGesture(); dragging = false; repaint(); }
    void mouseDoubleClick (const MouseEvent&) override { attach.setValueAsCompleteGesture (param.convertFrom0to1 (param.getDefaultValue())); }
    void mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w) override
    {
        if (w.deltaY == 0.0f) return;
        nudge ((w.deltaY > 0 ? 1.0f : -1.0f) * (e.mods.isShiftDown() ? 0.005f : 0.025f));
    }
    bool keyPressed (const KeyPress& k) override
    {
        const float st = k.getModifiers().isShiftDown() ? 0.005f : 0.02f;
        if (k.getKeyCode() == KeyPress::upKey || k.getKeyCode() == KeyPress::rightKey) { nudge (st); return true; }
        if (k.getKeyCode() == KeyPress::downKey || k.getKeyCode() == KeyPress::leftKey) { nudge (-st); return true; }
        return false;
    }
    void focusGained (FocusChangeType) override { repaint(); }
    void focusLost (FocusChangeType) override { repaint(); }

private:
    float value() const { return param.convertFrom0to1 (param.getValue()); }
    void nudge (float d) { attach.setValueAsCompleteGesture (param.convertFrom0to1 (jlimit (0.0f, 1.0f, param.getValue() + d))); }

    RangedAudioParameter& param;
    String name;
    float size;
    ParameterAttachment attach;
    bool dragging = false;
    int startY = 0;
    float startN = 0.0f;
    bool lit = false;
    Colour accent, accentPeak;
};

//==============================================================================
// Four-position rotary waveform switch with LEDs
class WaveSwitch : public Component
{
public:
    explicit WaveSwitch (RangedAudioParameter& p)
        : param (p), attach (p, [this] (float v) { index = jlimit (0, 3, (int) v); repaint(); }, nullptr)
    {
        attach.sendInitialUpdate();
        setTitle ("Waveform");
        setMouseCursor (MouseCursor::PointingHandCursor);
    }

    // centre of the switch inside this component
    Point<float> centre() const { return { 80.0f, 76.0f }; }

    void paint (Graphics& g) override
    {
        const auto c = centre();
        for (int i = 0; i < 4; ++i)
        {
            drawLed (g, Rectangle<float> (9, 9).withCentre (polar (c, 41, angles[i])), i == index);
            g.setColour (i == index ? col::ink : col::inkDim); g.setFont (label (12.5f, 0.1f));
            g.drawText (pk::waveNames[i], Rectangle<float> (40, 14).withCentre (polar (c, 58, angles[i])), Justification::centred, false);
        }
        drawLitKnob (g, c, 76.0f, (angles[index] + 135.0f) / 270.0f, Colour (0xffd9dde2), Colours::white, false, false);
        g.setColour (col::ink); g.setFont (label (12.5f, 0.12f));
        g.drawText ("WAVEFORM", Rectangle<float> (120, 16).withCentre ({ c.x, c.y + 48 }), Justification::centred, false);
    }

    void mouseDown (const MouseEvent& e) override
    {
        const auto c = centre();
        int target = (index + 1) % 4;
        float best = 16.0f;
        for (int i = 0; i < 4; ++i)
        {
            const float d = e.position.getDistanceFrom (polar (c, 58, angles[i]));
            if (d < best) { best = d; target = i; }
        }
        attach.setValueAsCompleteGesture ((float) target);
    }

private:
    RangedAudioParameter& param;
    ParameterAttachment attach;
    int index = 0;
    const float angles[4] { -66.0f, -22.0f, 22.0f, 66.0f };
};

//==============================================================================
// Bat-handle toggle switch with LED, bound to a bool parameter
class ToggleSwitch : public Component
{
public:
    ToggleSwitch (RangedAudioParameter& p, String title, String onText, String offText)
        : param (p), name (std::move (title)), onT (std::move (onText)), offT (std::move (offText)),
          attach (p, [this] (float v) { on = v > 0.5f; repaint(); }, nullptr)
    {
        attach.sendInitialUpdate();
        setTitle (name);
        setMouseCursor (MouseCursor::PointingHandCursor);
    }

    void placeAt (int cx, int cy) { setBounds (cx - 50, cy - 48, 100, 80); }

    void paint (Graphics& g) override
    {
        const float cx = 50.0f;
        drawLed (g, Rectangle<float> (11, 11).withCentre ({ cx, 7.5f }), on);
        const Point<float> c (cx, 38.0f);
        g.setColour (col::inkDim); g.setFont (label (12.0f, 0.08f));
        g.drawText (onT,  Rectangle<float> (0, c.y - 7, 30, 14), Justification::centredRight, false);
        g.drawText (offT, Rectangle<float> (70, c.y - 7, 30, 14), Justification::centredLeft, false);
        g.setColour (Colour (0xff0b0b0c)); g.fillEllipse (Rectangle<float> (30, 30).withCentre (c));
        g.setGradientFill (ColourGradient (Colour (0xffb5b5b8), c.x - 12, c.y - 12, Colour (0xff2a2a2d), c.x + 12, c.y + 12, false));
        g.fillEllipse (Rectangle<float> (24, 24).withCentre (c));
        g.setColour (Colour (0xff2a2a2d)); g.fillEllipse (Rectangle<float> (12, 12).withCentre (c));
        const auto tip = polar (c, 15.0f, on ? -32.0f : 32.0f);
        g.setColour (Colour (0xffd8d8da)); g.drawLine (c.x, c.y, tip.x, tip.y, 5.0f);
        g.setColour (Colour (0xffe9e9ea)); g.fillEllipse (Rectangle<float> (7.2f, 7.2f).withCentre (tip));
        g.setColour (col::ink); g.setFont (label (13.0f, 0.12f));
        g.drawText (name, Rectangle<float> (0, 60, 100, 16), Justification::centred, false);
    }

    void mouseDown (const MouseEvent&) override { attach.setValueAsCompleteGesture (on ? 0.0f : 1.0f); }

private:
    RangedAudioParameter& param;
    String name, onT, offT;
    ParameterAttachment attach;
    bool on = false;
};

//==============================================================================
// Amber VU meter for the output level
class VuMeter : public Component
{
public:
    void setLevel (float rms)
    {
        const float db = 20.0f * std::log10 (rms + 1.0e-6f) + 10.0f;   // 0 VU = -10 dBFS RMS
        level += (db - level) * (db > level ? 0.35f : 0.06f);
        repaint();
    }

    void paint (Graphics& g) override
    {
        const Point<float> pivot (125, 176);
        const float R = 118.0f;
        g.setColour (Colour (0xff060606)); g.fillRoundedRectangle (1, 1, 248, 148, 16);
        g.setColour (Colour (0xff26262a)); g.drawRoundedRectangle (1, 1, 248, 148, 16, 2);
        const Rectangle<float> face (13, 13, 224, 102);
        {
            Graphics::ScopedSaveState s (g);
            Path clip; clip.addRoundedRectangle (face, 7.0f);
            g.reduceClipRegion (clip);
            ColourGradient fg (Colour (0xfff9dc9b), 0, face.getY(), Colour (0xffdba253), 0, face.getBottom(), false);
            fg.addColour (0.6, Colour (0xfff0c072));
            g.setGradientFill (fg); g.fillRect (face);
            g.setGradientFill (ColourGradient (Colour (0x8cfff3c9), 125, 100, Colour (0x00fff3c9), 125 + 160, 100, true));
            g.fillRect (face);

            Path arc;  arc.addCentredArc (pivot.x, pivot.y, R, R, 0, degToRad (ang (-20)), degToRad (ang (0)), true);
            g.setColour (col::faceInk); g.strokePath (arc, PathStrokeType (1.2f));
            Path red;  red.addCentredArc (pivot.x, pivot.y, R, R, 0, degToRad (ang (0)), degToRad (ang (3)), true);
            g.setColour (col::faceRed); g.strokePath (red, PathStrokeType (4.0f));
            g.setFont (label (11.5f));
            for (int v : { -20, -10, -7, -5, -3, -2, -1, 0, 1, 2, 3 })
            {
                const float a = ang ((float) v);
                const auto p1 = polar (pivot, R - 9, a), p2 = polar (pivot, R, a), pt = polar (pivot, R + 11, a);
                g.setColour (v > 0 ? col::faceRed : col::faceInk);
                g.drawLine (p1.x, p1.y, p2.x, p2.y, 1.4f);
                g.drawText (v > 0 ? "+" + String (v) : String (v), Rectangle<float> (24, 12).withCentre (pt), Justification::centred, false);
            }
            g.setFont (label (13.5f, 0.0f, true));
            g.setColour (col::faceInk); g.drawText ("VU", Rectangle<float> (30, 22, 30, 14), Justification::left, false);
            g.setColour (col::faceRed); g.drawText ("VU", Rectangle<float> (190, 22, 30, 14), Justification::right, false);
            g.setColour (Colour (0xff6b4c1e)); g.setFont (label (9.0f, 0.25f));
            g.drawText ("POTATO KICKER", Rectangle<float> (60, 90, 130, 12), Justification::centred, false);
            const auto tip = polar (pivot, R + 4, ang (jlimit (-20.0f, 3.5f, level)));
            g.setColour (Colour (0xff18120b)); g.drawLine (pivot.x, pivot.y, tip.x, tip.y, 1.6f);
        }
        g.setColour (col::inkDim); g.setFont (label (12.5f, 0.3f));
        g.drawText ("OUTPUT", Rectangle<float> (60, 126, 130, 16), Justification::centred, false);
        drawLed (g, Rectangle<float> (11, 11).withCentre ({ 236.5f, 126.5f }), level > 0.0f);
    }

private:
    static float ang (float v)
    {
        const float p = (std::pow (10.0f, v / 20.0f) - 0.1f) / (1.41254f - 0.1f);
        return -46.0f + 92.0f * jlimit (0.0f, 1.0f, p);
    }
    float level = -25.0f;
};

//==============================================================================
// Envelope / pitch / compressor display with draggable points
class EnvDisplay : public Component
{
public:
    explicit EnvDisplay (PotatoKickerProcessor& p) : proc (p)
    {
        for (auto* pid : { pk::id::tune, pk::id::pamt, pk::id::pdec, pk::id::atk, pk::id::dec, pk::id::sus, pk::id::rel,
                           pk::id::thr, pk::id::ratio, pk::id::makeup })
            params.set (pid, proc.apvts.getParameter (pid));
        setTitle ("Sound display");
    }

    void showView (int v) { view = jlimit (0, 3, v); repaint(); }

    void tick()
    {
        const auto now = proc.readParams();
        if (! (now == last) || preview.empty())
        {
            last = now;
            renderPreview();
        }
        const auto cnt = proc.noteOnCounter.load();
        if (cnt != lastCounter) { lastCounter = cnt; playStart = Time::getMillisecondCounterHiRes(); }
        grShow += (proc.meterGr.load() - grShow) * 0.2f;
        feedWave();
        repaint();
    }

    void paint (Graphics& g) override
    {
        // bezel
        g.setGradientFill (ColourGradient (Colour (0xff0c0c0d), 0, 0, Colour (0xff050505), 0, (float) getHeight(), false));
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 16.0f);
        g.setColour (Colour (0xff26262a)); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 16.0f, 2.0f);

        // tabs
        const char* names[4] { "AMP", "PITCH", "COMP", "WAVE" };
        for (int i = 0; i < 4; ++i)
        {
            const auto r = tabRect (i);
            if (i == view)
            {
                g.setColour (col::cream.withAlpha (0.25f)); g.fillRoundedRectangle (r.expanded (3), 5);
                g.setGradientFill (ColourGradient (Colour (0xfffff6dc), r.getCentreX(), r.getCentreY() - 3, Colour (0xffd9b979), r.getRight(), r.getBottom(), true));
                g.fillRoundedRectangle (r, 3);
                g.setColour (Colour (0xff2a1c06));
            }
            else
            {
                g.setGradientFill (ColourGradient (Colour (0xff3a3a3e), 0, r.getY(), Colour (0xff1d1d20), 0, r.getBottom(), false));
                g.fillRoundedRectangle (r, 3);
                g.setColour (Colours::black); g.drawRoundedRectangle (r, 3, 1);
                g.setColour (Colour (0xff9a968e));
            }
            g.setFont (label (13.0f, 0.14f));
            g.drawText (names[i], r, Justification::centred, false);
        }
        g.setColour (col::amber); g.setFont (mono (13.0f));
        g.drawText (pk::noteName (last.tune) + " · " + String (last.tune, 1) + " Hz · " + pk::waveNames[last.wave],
                    Rectangle<float> (300, 12, 242, 22), Justification::centredRight, false);

        // screen
        {
            Graphics::ScopedSaveState s (g);
            g.setOrigin (scr.getPosition());
            g.reduceClipRegion (Rectangle<int> (0, 0, CW, CH));
            drawScreen (g);
        }
        g.setColour (Colours::black); g.drawRoundedRectangle (scr.toFloat(), 7.0f, 1.0f);

        // footer
        const char* foot[4] { "Drag ATTACK, DECAY/SUSTAIN and RELEASE points", "Drag START, PITCH TIME and TUNE points", "Drag THRESHOLD/MAKEUP and RATIO points", "Live output, last 2.4 seconds" };
        g.setColour (Colour (0xff7d6a4c)); g.setFont (mono (11.5f));
        g.drawText (foot[view], Rectangle<float> (22, 312, 400, 16), Justification::centredLeft, false);
        const String right = view == 0 ? (last.oneShot() ? "one-shot kick" : "gated bass")
                           : view == 1 ? "+" + String (roundToInt (last.pamt)) + " st"
                           : view == 2 ? String (last.ratio, 1) + ":1"
                           : "peak " + (wavePeakDb > -60.0f ? String (wavePeakDb, 1) + " dB" : String ("-inf"));
        g.drawText (right, Rectangle<float> (342, 312, 200, 16), Justification::centredRight, false);
    }

    void mouseDown (const MouseEvent& e) override
    {
        for (int i = 0; i < 4; ++i)
            if (tabRect (i).contains (e.position)) { view = i; hover = {}; repaint(); return; }

        const auto p = toScreen (e.position);
        const auto id = hitNode (p);
        if (id.isEmpty()) return;
        drag = id; dragT = T; dragGate = gate;
        for (auto* prm : paramsFor (id)) prm->beginChangeGesture();
    }

    void mouseDrag (const MouseEvent& e) override
    {
        if (drag.isEmpty()) return;
        setMouseCursor (MouseCursor::DraggingHandCursor);
        const auto p = toScreen (e.position);
        const float t = xt (jlimit ((float) PL, (float) (PL + GW), p.x));
        const auto& v = last;
        if (drag == "A")       set (pk::id::atk, t * 1000.0f);
        else if (drag == "D")  { set (pk::id::dec, t * 1000.0f - v.atk); set (pk::id::sus, ya (p.y) * 100.0f); }
        else if (drag == "R")  set (pk::id::rel, (t - dragGate) * 1000.0f);
        else if (drag == "P0") set (pk::id::pamt, 12.0f * std::log2 (yf (p.y) / v.tune));
        else if (drag == "P1") set (pk::id::pdec, t * 1000.0f);
        else if (drag == "P2") set (pk::id::tune, yf (p.y));
        else if (drag == "T")  { const float th = jlimit (-40.0f, 0.0f, xcd (p.x)); set (pk::id::thr, th); set (pk::id::makeup, ycd (p.y) - th); }
        else if (drag == "Q")
        {
            const float o = ycd (p.y) - v.makeup;
            if (v.thr < -0.5f) set (pk::id::ratio, o > v.thr + 0.01f ? -v.thr / (o - v.thr) : 20.0f);
        }
        tick();
    }

    void mouseUp (const MouseEvent&) override
    {
        if (drag.isNotEmpty())
            for (auto* prm : paramsFor (drag)) prm->endChangeGesture();
        drag = {};
        setMouseCursor (hover.isNotEmpty() ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor);
    }

    void mouseMove (const MouseEvent& e) override
    {
        const auto h = scr.toFloat().contains (e.position) ? hitNode (toScreen (e.position)) : String();
        if (h != hover)
        {
            hover = h;
            setMouseCursor (h.isNotEmpty() ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor);
            repaint();
        }
    }
    void mouseExit (const MouseEvent&) override { if (drag.isEmpty()) hover = {}; }

private:
    static constexpr int CW = 520, CH = 262, PL = 40, PR = 12, PT = 12, PB = 22, GW = CW - PL - PR, GH = CH - PT - PB;
    const Rectangle<int> scr { 22, 44, CW, CH };

    struct Node { String id; float x, y; String text; };

    Rectangle<float> tabRect (int i) const
    {
        const float w[4] { 54, 66, 64, 62 };
        float x = 22;
        for (int j = 0; j < i; ++j) x += w[j] + 8;
        return { x, 14, w[i], 22 };
    }

    Point<float> toScreen (Point<float> p) const { return p - scr.getPosition().toFloat(); }

    void renderPreview()
    {
        auto p = last;
        p.keytrack = false; p.velocity = false; p.stereo = false;
        const float A = p.atk * 0.001f, D = p.dec * 0.001f, R = p.rel * 0.001f;
        oneShot = p.oneShot();
        gate = oneShot ? A + D : A + D + jmax (0.15f, (A + D) * 0.3f);
        float len = oneShot ? (A + D) * 1.08f + 0.01f : gate + R * 1.04f;
        len = jlimit (0.08f, 5.0f, len);
        T = drag.isNotEmpty() ? dragT : len;
        if (drag.isNotEmpty()) gate = dragGate;

        constexpr double sr = 16000.0;
        const int N = (int) std::ceil (T * sr);
        preview.assign ((size_t) N, 0.0f);
        pk::KickEngine e;
        e.prepare (sr, 4242u);
        e.setParams (p);
        e.noteOn (36, 127);
        float tmp[256], sumSq = 0.0f, gr = 0.0f;
        bool off = false;
        for (int pos = 0; pos < N; pos += 256)
        {
            const int n = jmin (256, N - pos);
            if (! oneShot && ! off && (float) pos / (float) sr >= gate) { e.noteOff (36); off = true; }
            e.render (tmp, nullptr, n, sumSq, gr);
            std::copy (tmp, tmp + n, preview.begin() + pos);
        }
    }

    // coordinate helpers (screen space)
    float tx (float t) const { return PL + t / T * GW; }
    float xt (float x) const { return (x - PL) / GW * T; }
    float ay (float e) const { return PT + GH * 0.5f - e * GH * 0.5f * 0.92f; }
    float ya (float y) const { return (PT + GH * 0.5f - y) / (GH * 0.5f * 0.92f); }
    float fy (float f) const { return PT + GH - std::log (f / 20.0f) / std::log (100.0f) * GH; }
    float yf (float y) const { return 20.0f * std::pow (100.0f, (PT + GH - y) / GH); }
    float cxd (float d) const { return PL + (d + 48.0f) / 48.0f * GW; }
    float cyd (float d) const { return PT + GH - (d + 48.0f) / 60.0f * GH; }
    float xcd (float x) const { return (x - PL) / GW * 48.0f - 48.0f; }
    float ycd (float y) const { return (PT + GH - y) / GH * 60.0f - 48.0f; }
    float compOut (float d) const { return (d <= last.thr ? d : last.thr + (d - last.thr) / last.ratio) + last.makeup; }

    float envDisp (float t) const
    {
        if (oneShot || t < gate) return last.envHold (t);
        const float r = (t - gate) / jmax (0.005f, last.rel * 0.001f);
        return r >= 1.0f ? 0.0f : last.envHold (gate) * (1.0f - r) * (1.0f - r);
    }

    std::vector<Node> nodes() const
    {
        const auto& v = last;
        if (view == 0)
            return { { "A", tx (v.atk * 0.001f), ay (1.0f), "ATTACK " + pk::fmtMs (v.atk) },
                     { "D", tx ((v.atk + v.dec) * 0.001f), ay (v.sus * 0.01f), "DECAY " + pk::fmtMs (v.dec) + " · SUSTAIN " + String (roundToInt (v.sus)) + "%" },
                     { "R", tx (gate + v.rel * 0.001f), ay (0.0f), "RELEASE " + pk::fmtMs (v.rel) } };
        if (view == 1)
        {
            const float f0 = v.tune * std::exp2 (v.pamt / 12.0f);
            return { { "P0", tx (0.0f), fy (f0), "START +" + String (roundToInt (v.pamt)) + " st · " + String (roundToInt (f0)) + " Hz" },
                     { "P1", tx (v.pdec * 0.001f), fy (v.pitchAt (v.pdec * 0.001f)), "PITCH TIME " + pk::fmtMs (v.pdec) },
                     { "P2", PL + GW * 0.92f, fy (v.tune), "TUNE " + pk::formatValue (pk::id::tune, v.tune) } };
        }
        if (view != 2) return {};
        return { { "T", cxd (v.thr), cyd (v.thr + v.makeup), "THRESHOLD " + String (v.thr, 1) + " dB · MAKEUP +" + String (v.makeup, 1) },
                 { "Q", cxd (0.0f), cyd (compOut (0.0f)), "RATIO " + String (v.ratio, 1) + ":1" } };
    }

    String hitNode (Point<float> p) const
    {
        String best; float bd = 16.0f;
        for (auto& n : nodes())
        {
            const float d = p.getDistanceFrom ({ n.x, n.y });
            if (d < bd) { bd = d; best = n.id; }
        }
        return best;
    }

    Array<RangedAudioParameter*> paramsFor (const String& id) const
    {
        if (id == "A")  return { params[pk::id::atk] };
        if (id == "D")  return { params[pk::id::dec], params[pk::id::sus] };
        if (id == "R")  return { params[pk::id::rel] };
        if (id == "P0") return { params[pk::id::pamt] };
        if (id == "P1") return { params[pk::id::pdec] };
        if (id == "P2") return { params[pk::id::tune] };
        if (id == "T")  return { params[pk::id::thr], params[pk::id::makeup] };
        return { params[pk::id::ratio] };
    }

    void set (const char* pid, float v)
    {
        auto* prm = params[pid];
        prm->setValueNotifyingHost (prm->convertTo0to1 (prm->getNormalisableRange().snapToLegalValue (v)));
    }

    static float niceStep (float t)
    {
        for (float s : { 0.005f, 0.01f, 0.02f, 0.05f, 0.1f, 0.2f, 0.25f, 0.5f, 1.0f })
            if (t / s <= 8.0f) return s;
        return 1.0f;
    }

    void drawScreen (Graphics& g)
    {
        if (view == 3) { drawWave (g); return; }
        g.setGradientFill (ColourGradient (Colour (0xff1e1408), CW * 0.5f, CH * 0.9f, Colour (0xff0a0703), CW * 0.5f + CW * 0.7f, CH * 0.9f, true));
        g.fillRect (0, 0, CW, CH);

        const Colour grid = col::amber.withAlpha (0.08f), lbl = col::amber.withAlpha (0.45f);
        g.setFont (mono (10.5f));
        auto hline = [&] (float y) { g.setColour (grid); g.drawHorizontalLine (roundToInt (y), (float) PL, (float) (PL + GW)); };
        auto vline = [&] (float x) { g.setColour (grid); g.drawVerticalLine (roundToInt (x), (float) PT, (float) (PT + GH)); };
        auto text  = [&] (const String& s, float x, float y, Justification j)
        {
            g.setColour (lbl);
            if (j == Justification::centredRight) g.drawText (s, Rectangle<float> (0, y - 7, (float) PL - 5, 14), j, false);  // y-axis label
            else                                  g.drawText (s, Rectangle<float> (x - 40, y - 7, 80, 14), j, false);
        };

        if (view < 2)
        {
            const float st = niceStep (T);
            for (float t = 0.0f; t <= T + 1.0e-6f; t += st)
            {
                vline (tx (t));
                text (t < 1.0f ? String (roundToInt (t * 1000.0f)) + "ms" : String (t, 2) + "s", tx (t), CH - 9.0f, Justification::centred);
            }
        }
        if (view == 0)
        {
            for (float e : { 1.0f, 0.5f, 0.0f, -0.5f, -1.0f }) hline (ay (e));
            text ("100%", PL - 44.0f, ay (1.0f), Justification::centredRight);
            text ("50%",  PL - 44.0f, ay (0.5f), Justification::centredRight);
            text ("0",    PL - 44.0f, ay (0.0f), Justification::centredRight);
        }
        if (view == 1)
            for (float f : { 20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f })
            {
                hline (fy (f));
                text (f >= 1000.0f ? String (roundToInt (f / 1000.0f)) + "k" : String (roundToInt (f)), PL - 44.0f, fy (f), Justification::centredRight);
            }
        if (view == 2)
        {
            for (int d = -48; d <= 0; d += 12)  { vline (cxd ((float) d)); text (String (d), cxd ((float) d), CH - 9.0f, Justification::centred); }
            for (int d = -48; d <= 12; d += 12) { hline (cyd ((float) d)); text (String (d), PL - 44.0f, cyd ((float) d), Justification::centredRight); }
        }

        // waveform
        if (view < 2 && ! preview.empty())
        {
            const int N = (int) preview.size();
            const float alpha = view == 0 ? 0.85f : 0.22f;
            for (int c = 0; c < GW; ++c)
            {
                const int i0 = (int) ((float) c / GW * N), i1 = jmax (i0 + 1, (int) ((float) (c + 1) / GW * N));
                float mn = 1.0f, mx = -1.0f;
                for (int i = i0; i < i1 && i < N; ++i) { mn = jmin (mn, preview[(size_t) i]); mx = jmax (mx, preview[(size_t) i]); }
                if (mx < mn) continue;
                const float top = ay (mx), bot = ay (mn) + 0.6f;
                if (view == 0) { g.setColour (Colour (0xffffaa3c).withAlpha (0.12f)); g.fillRect ((float) (PL + c) - 1.0f, top - 1.5f, 3.0f, bot - top + 3.0f); }
                g.setColour (col::amber.withAlpha (alpha));
                g.drawVerticalLine (PL + c, top, bot);
            }
        }

        if (view == 0)
        {
            Path env;
            for (int c = 0; c <= GW; c += 2)
            {
                const float y = ay (envDisp (xt ((float) (PL + c))));
                if (c == 0) env.startNewSubPath ((float) PL, y); else env.lineTo ((float) (PL + c), y);
            }
            g.setColour (Colour (0xfffff1cf)); g.strokePath (env, PathStrokeType (1.6f));
            g.setFont (mono (10.5f));
            if (! oneShot)
            {
                const float x = tx (gate);
                Path dash; dash.startNewSubPath (x, (float) PT); dash.lineTo (x, (float) (PT + GH));
                const float pat[] { 3.0f, 4.0f };
                PathStrokeType (1.0f).createDashedStroke (dash, dash, pat, 2);
                g.setColour (Colour (0xffff5a46).withAlpha (0.7f)); g.fillPath (dash);
                g.setColour (Colour (0xffff5a46).withAlpha (0.85f));
                g.drawText ("NOTE OFF", Rectangle<float> (x + 4, (float) PT + 2, 80, 12), Justification::centredLeft, false);
            }
            else
            {
                g.setColour (col::amber.withAlpha (0.6f));
                g.drawText ("ONE-SHOT", Rectangle<float> ((float) (PL + GW - 104), (float) PT + 2, 100, 12), Justification::centredRight, false);
            }
        }

        if (view == 1)
        {
            const float yt = fy (last.tune);
            Path dash; dash.startNewSubPath ((float) PL, yt); dash.lineTo ((float) (PL + GW), yt);
            const float pat[] { 3.0f, 4.0f };
            PathStrokeType (1.0f).createDashedStroke (dash, dash, pat, 2);
            g.setColour (Colour (0xffff5a46).withAlpha (0.6f)); g.fillPath (dash);
            Path curve;
            for (int c = 0; c <= GW; c += 2)
            {
                const float y = fy (last.pitchAt (xt ((float) (PL + c))));
                if (c == 0) curve.startNewSubPath ((float) PL, y); else curve.lineTo ((float) (PL + c), y);
            }
            g.setColour (col::amberHi.withAlpha (0.25f)); g.strokePath (curve, PathStrokeType (5.0f));
            g.setColour (col::amberHi); g.strokePath (curve, PathStrokeType (2.0f));
            g.setColour (Colour (0xffff5a46).withAlpha (0.85f)); g.setFont (mono (10.5f));
            g.drawText (pk::noteName (last.tune), Rectangle<float> ((float) PL + 4, yt - 16, 60, 12), Justification::centredLeft, false);
        }

        if (view == 2)
        {
            Path unity; unity.startNewSubPath (cxd (-48), cyd (-48)); unity.lineTo (cxd (0), cyd (0));
            const float pat[] { 3.0f, 4.0f };
            PathStrokeType (1.0f).createDashedStroke (unity, unity, pat, 2);
            g.setColour (col::amber.withAlpha (0.25f)); g.fillPath (unity);
            Path curve;
            for (float d = -48.0f; d <= 0.0f; d += 0.5f)
            {
                const auto pnt = Point<float> (cxd (d), cyd (last.comp ? compOut (d) : d));
                if (d == -48.0f) curve.startNewSubPath (pnt); else curve.lineTo (pnt);
            }
            const float a = last.comp ? 1.0f : 0.35f;
            g.setColour (col::amberHi.withAlpha (0.25f * a)); g.strokePath (curve, PathStrokeType (5.0f));
            g.setColour (col::amberHi.withAlpha (a)); g.strokePath (curve, PathStrokeType (2.0f));
            const float h = jmin ((float) GH, grShow / 24.0f * GH);
            g.setColour (col::led.withAlpha (0.15f)); g.fillRect ((float) (PL + GW - 16), (float) PT, 10.0f, (float) GH);
            g.setColour (Colour (0xffff3b2c)); g.fillRect ((float) (PL + GW - 16), (float) PT, 10.0f, h);
            g.setColour (col::amber.withAlpha (0.6f)); g.setFont (mono (10.5f));
            g.drawText (last.comp ? "GR " + String (grShow, 1) + " dB" : "BYPASSED", Rectangle<float> ((float) (PL + GW - 140), (float) PT + 2, 116, 12), Justification::centredRight, false);
            g.drawText (CharPointer_UTF8 ("IN dB \xe2\x86\x92"), Rectangle<float> ((float) PL + 4, (float) (PT + GH - 14), 80, 12), Justification::centredLeft, false);
        }

        // playhead
        if (view < 2 && playStart > 0.0)
        {
            const float t = (float) ((Time::getMillisecondCounterHiRes() - playStart) / 1000.0);
            if (t < T)
            {
                g.setColour (Colours::white.withAlpha (0.55f));
                g.drawVerticalLine (roundToInt (tx (t)), (float) PT, (float) (PT + GH));
            }
        }

        // nodes
        const String active = drag.isNotEmpty() ? drag : hover;
        for (auto& n : nodes())
        {
            const bool on = n.id == active;
            const auto r = Rectangle<float> (on ? 14.0f : 11.0f, on ? 14.0f : 11.0f).withCentre ({ n.x, n.y });
            g.setColour (on ? Colour (0xffff3b2c) : Colour (0xff140d05)); g.fillEllipse (r);
            g.setColour (on ? Colour (0xffffd2c8) : col::amberHi); g.drawEllipse (r, 2.0f);
            if (on)
            {
                g.setFont (mono (12.0f));
                const float w = GlyphArrangement::getStringWidth (g.getCurrentFont(), n.text) + 14.0f;
                const float bx = jlimit ((float) PL, (float) (CW - PR) - w, n.x - w * 0.5f);
                const float by = n.y - 34.0f < PT ? n.y + 14.0f : n.y - 34.0f;
                g.setColour (Colour (0xeb0a0703)); g.fillRect (bx, by, w, 20.0f);
                g.setColour (col::amber.withAlpha (0.5f)); g.drawRect (bx, by, w, 20.0f, 1.0f);
                g.setColour (col::amberHi); g.drawText (n.text, Rectangle<float> (bx + 7, by, w - 7, 20), Justification::centredLeft, false);
            }
        }
    }


    //==========================================================================
    // WAVE view: scrolling live output in the Magic Touch waveform style.
    // Each 1-px column is coloured by how the energy splits across six bands (sub at the edge, air at the
    // centre), kicks pull the lows toward plum, and the top half is mirrored below a glowing horizon.
    static constexpr float wavePxPerSec = 200.0f;

    void feedWave()
    {
        const int pos = proc.getScopeWritePos();
        if (view != 3 || waveLastPos < 0) { waveLastPos = pos; if (view != 3) return; }
        const double srate = proc.getSampleRate() > 0.0 ? proc.getSampleRate() : 48000.0;
        const int w = GW, h = GH;
        if (! waveImg.isValid()) waveImg = Image (Image::ARGB, w, h, true);

        int n = (pos - waveLastPos) & (PotatoKickerProcessor::scopeSize - 1);
        waveLastPos = pos;
        n = jmin (n, PotatoKickerProcessor::scopeSize / 2);
        if (n <= 0) return;
        waveBuf.resize ((size_t) n);
        proc.copyScope (waveBuf.data(), n, pos);

        const int samplesPerCol = jmax (1, (int) std::round (srate / (double) wavePxPerSec));
        auto pole = [srate] (float hz) { return std::exp (-MathConstants<float>::twoPi * hz / (float) srate); };
        const float c[5] = { pole (60.0f), pole (250.0f), pole (500.0f), pole (2000.0f), pole (6000.0f) };
        auto envCoef = [srate] (float secs) { return std::exp (-1.0f / ((float) srate * secs)); };
        const float kFastA = envCoef (0.001f), kFastR = envCoef (0.030f), kSlowA = envCoef (0.060f), kSlowR = envCoef (0.300f);
        const float kHoldDecay = envCoef (0.10f);
        const float cy = (float) h * 0.5f, half = (float) h * 0.47f;

        static const Colour bands[6] { Colour (0xff4a2fd0), Colour (0xff2f8dff), Colour (0xffc040d8),
                                       Colour (0xffff5c78), Colour (0xffffa63e), Colour (0xffffe8c2) };
        static const Colour kickCol (0xff230a36), edgeCol (0xffffb46e);
        static const float gain[6] { 1.0f, 1.0f, 1.3f, 1.6f, 2.2f, 3.0f };

        for (int i = 0; i < n; ++i)
        {
            const float x = waveBuf[(size_t) i];
            float lp[5];
            for (int k = 0; k < 5; ++k)
            {
                auto& s1 = waveLp[(size_t) (k * 2)];
                auto& s2 = waveLp[(size_t) (k * 2 + 1)];
                s1 = x + c[k] * (s1 - x);
                s2 = s1 + c[k] * (s2 - s1);
                lp[k] = s2;
            }
            const float band[6] { lp[0], lp[1] - lp[0], lp[2] - lp[1], lp[3] - lp[2], lp[4] - lp[3], x - lp[4] };
            for (size_t k = 0; k < 6; ++k) wBand[k] += band[k] * band[k];

            const float lo = std::abs (lp[1]);
            kickFast = lo + (lo > kickFast ? kFastA : kFastR) * (kickFast - lo);
            kickSlow = lo + (lo > kickSlow ? kSlowA : kSlowR) * (kickSlow - lo);
            const float kickness = jlimit (0.0f, 1.0f, (kickFast - 1.25f * kickSlow) / (kickFast + 1.0e-6f) * 3.0f);
            kickHold = jmax (kickness, kickHold * kHoldDecay);
            wKick = jmax (wKick, kickHold);
            wPeak = jmax (wPeak, std::abs (x));
            peakHold = jmax (peakHold * 0.99997f, std::abs (x));

            if (++waveColCount < samplesPerCol) continue;

            float e[6], sum = 1.0e-9f, ms = 0.0f;
            for (size_t k = 0; k < 6; ++k)
            {
                ms += wBand[k];
                e[k] = std::sqrt (wBand[k] / (float) samplesPerCol) * gain[k];
                sum += e[k];
            }
            for (size_t k = 0; k < 6; ++k) waveShare[k] += (e[k] / sum - waveShare[k]) * 0.35f;
            waveKickSm += (wKick - waveKickSm) * 0.5f;

            const float rms = std::sqrt (ms / (float) samplesPerCol);
            auto heightOf = [&] (float amp) { return half * std::pow (jlimit (0.0f, 1.0f, amp), 0.75f); };
            const float body = jmax (1.0f, heightOf (rms * 1.6f));
            const float halo = jmax (body, heightOf (wPeak));

            ColourGradient grad;
            grad.isRadial = false;
            float acc = 0.0f;
            bool first = true;
            for (int k = 5; k >= 0; --k)
            {
                const float sh = waveShare[(size_t) k];
                auto col = bands[k];
                if (k <= 1) col = col.interpolatedWith (kickCol, jlimit (0.0f, 0.75f, waveKickSm * 0.75f));
                const float p = jlimit (0.0f, 1.0f, acc + sh * 0.5f);
                if (first) { grad.addColour (0.0, col.brighter (0.25f)); first = false; }
                if (p > 0.0f && p < 1.0f) grad.addColour ((double) p, col);
                acc += sh;
            }
            const auto outer = bands[0].interpolatedWith (kickCol, jlimit (0.0f, 0.75f, waveKickSm * 0.75f));
            grad.addColour (1.0, outer.interpolatedWith (edgeCol, 0.35f));

            waveImg.moveImageSection (0, 0, 1, 0, w - 1, h);
            waveImg.clear ({ w - 1, 0, 1, h });
            Graphics gi (waveImg);
            const float xc = (float) (w - 1);
            for (int side = -1; side <= 1; side += 2)
            {
                auto gr = grad;
                gr.point1 = { xc, cy };
                gr.point2 = { xc, cy + side * body };
                gi.setGradientFill (gr);
                gi.fillRect (xc, side < 0 ? cy - body : cy, 1.0f, body);
                if (halo > body + 0.5f)
                {
                    gi.setGradientFill (ColourGradient (edgeCol.withAlpha (0.38f), xc, cy + side * body, edgeCol.withAlpha (0.0f), xc, cy + side * halo, false));
                    gi.fillRect (xc, side < 0 ? cy - halo : cy + body, 1.0f, halo - body);
                }
            }
            waveColCount = 0;
            wPeak = wKick = 0.0f;
            wBand.fill (0.0f);
        }
        wavePeakDb = 20.0f * std::log10 (peakHold + 1.0e-9f);
    }

    void drawWave (Graphics& g)
    {
        g.setGradientFill (ColourGradient (Colour (0xff0b0d14), 0, 0, Colour (0xff030407), 0, (float) CH, false));
        g.fillRect (0, 0, CW, CH);

        // mirrored dB guides
        const float cy = PT + GH * 0.5f, half = GH * 0.47f;
        g.setFont (mono (10.5f));
        for (int db : { 0, -6, -12, -24 })
        {
            const float off = half * std::pow (std::pow (10.0f, db / 20.0f), 0.75f);
            for (int side = -1; side <= 1; side += 2)
            {
                const float y = std::round (cy + side * off);
                g.setColour (Colours::white.withAlpha (db == 0 ? 0.10f : 0.06f));
                g.drawHorizontalLine ((int) y, (float) PL, (float) (PL + GW));
                g.setColour (Colours::white.withAlpha (0.32f));
                g.drawText (String (db), Rectangle<float> (0, y - 7, (float) PL - 5, 14), Justification::centredRight, false);
            }
        }
        for (int s = 0; s <= 2; ++s)   // one second per division, newest at the right
        {
            const float x = (float) (PL + GW) - s * wavePxPerSec;
            g.setColour (Colours::white.withAlpha (0.05f)); g.drawVerticalLine (roundToInt (x), (float) PT, (float) (PT + GH));
            g.setColour (Colours::white.withAlpha (0.32f));
            g.drawText (s == 0 ? String ("now") : "-" + String (s) + "s", Rectangle<float> (x - 40, CH - 16.0f, 40, 14), Justification::centredRight, false);
        }

        if (waveImg.isValid()) g.drawImageAt (waveImg, PL, PT);

        // glowing horizon: white-hot core fading through amber to blue at the edges
        ColourGradient hz (Colour (0xff2e8bff).withAlpha (0.0f), (float) PL, cy, Colour (0xff2e8bff).withAlpha (0.0f), (float) (PL + GW), cy, false);
        hz.addColour (0.20, Colour (0xffffa63e).withAlpha (0.22f));
        hz.addColour (0.45, Colour (0xffffe8c2).withAlpha (0.38f));
        hz.addColour (0.75, Colour (0xff8fb8ff).withAlpha (0.22f));
        g.setGradientFill (hz); g.fillRect ((float) PL, cy - 3.0f, (float) GW, 7.0f);
        hz.multiplyOpacity (1.8f);
        g.setGradientFill (hz); g.fillRect ((float) PL, cy, (float) GW, 1.0f);

        // "now" line at the right edge
        const float x = (float) (PL + GW) - 1.0f;
        g.setColour (Colour (0xff3ba7ff).withAlpha (0.25f)); g.fillRect (x - 3.0f, (float) PT, 6.0f, (float) GH);
        g.setColour (Colour (0xff3ba7ff).brighter (0.6f));    g.fillRect (x, (float) PT, 1.0f, (float) GH);
    }

    Image waveImg;
    std::vector<float> waveBuf;
    int waveLastPos = -1, waveColCount = 0;
    std::array<float, 10> waveLp {};
    std::array<float, 6> wBand {}, waveShare { 1.0f / 6, 1.0f / 6, 1.0f / 6, 1.0f / 6, 1.0f / 6, 1.0f / 6 };
    float kickFast = 0.0f, kickSlow = 0.0f, kickHold = 0.0f, wKick = 0.0f, wPeak = 0.0f, waveKickSm = 0.0f;
    float peakHold = 0.0f, wavePeakDb = -90.0f;

    PotatoKickerProcessor& proc;
    HashMap<String, RangedAudioParameter*> params;
    pk::EngineParams last;
    std::vector<float> preview;
    float T = 0.3f, gate = 0.3f, dragT = 0.3f, dragGate = 0.3f, grShow = 0.0f;
    bool oneShot = true;
    int view = 0;
    String drag, hover;
    uint32_t lastCounter = 0;
    double playStart = 0.0;
};

//==============================================================================
// Strip widgets
class SquareButton : public Button
{
public:
    enum Kind { arrowLeft, arrowRight, text };
    SquareButton (Kind k, String t = {}) : Button (t), kind (k) { setTitle (t); }

    void paintButton (Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        if (down) r.translate (0, 1);
        g.setColour (Colours::black.withAlpha (0.6f)); g.fillRoundedRectangle (r.translated (0, 2), 3);
        if (getToggleState())
        {
            g.setColour (col::cream.withAlpha (0.25f)); g.fillRoundedRectangle (r.expanded (3), 5);
            g.setGradientFill (ColourGradient (Colour (0xfffff8e4), r.getCentreX(), r.getCentreY() - 3, Colour (0xffd7b06a), r.getRight(), r.getBottom(), true));
        }
        else
            g.setGradientFill (ColourGradient (Colour (over ? 0xff86868b : 0xff77777c), 0, r.getY(), Colour (0xff3b3b3f), 0, r.getBottom(), false));
        g.fillRoundedRectangle (r, 3);
        g.setColour (Colours::black); g.drawRoundedRectangle (r, 3, 1);
        g.setColour (Colour (0xff151515));
        if (kind == text)
        {
            g.setFont (label (13.0f, 0.08f, true));
            g.drawText (getButtonText(), r, Justification::centred, false);
        }
        else
        {
            const auto c = r.getCentre();
            Path tri;
            if (kind == arrowLeft) tri.addTriangle (c.x - 5, c.y, c.x + 4, c.y - 5.5f, c.x + 4, c.y + 5.5f);
            else                   tri.addTriangle (c.x + 5, c.y, c.x - 4, c.y - 5.5f, c.x - 4, c.y + 5.5f);
            g.fillPath (tri);
        }
    }

private:
    Kind kind;
};

class LedWindow : public Button
{
public:
    LedWindow (bool redStyle) : Button ("window"), red (redStyle) {}
    void setText (const String& s) { if (s != txt) { txt = s; repaint(); } }
    void paintButton (Graphics& g, bool over, bool) override
    {
        const auto r = getLocalBounds().toFloat().reduced (1.0f);
        insetWindow (g, r);
        if (over) { g.setColour (Colours::white.withAlpha (0.03f)); g.fillRoundedRectangle (r, 3); }
        if (red)
        {
            g.setFont (mono (18.0f));
            glowText (g, txt, r, Justification::centred, Colour (0xffff4232), 0.25f);
        }
        else
        {
            g.setFont (mono (17.0f));
            glowText (g, txt, r.withTrimmedLeft (12).withTrimmedRight (10), Justification::centredLeft, col::amber, 0.18f);
        }
    }
private:
    bool red;
    String txt;
};

class SevenSeg : public Component
{
public:
    void setNumber (int n) { if (n != num) { num = n; repaint(); } }
    void paint (Graphics& g) override
    {
        insetWindow (g, getLocalBounds().toFloat().reduced (1.0f));
        static const char* segs[10] { "abcdef", "bc", "abged", "abgcd", "fgbc", "afgcd", "afgedc", "abc", "abcdefg", "abfgcd" };
        struct R { char k; float x, y, w, h; };
        const R rs[7] { { 'a', 5, 1, 13, 3.2f }, { 'b', 18.5f, 4, 3.2f, 13 }, { 'c', 18.5f, 21, 3.2f, 13 }, { 'd', 5, 34.8f, 13, 3.2f },
                        { 'e', 1.3f, 21, 3.2f, 13 }, { 'f', 1.3f, 4, 3.2f, 13 }, { 'g', 5, 17.9f, 13, 3.2f } };
        const String s = String (num).paddedLeft ('0', 2);
        for (int i = 0; i < 2; ++i)
        {
            const String on (segs[s[i] - '0']);
            const auto xf = AffineTransform::shear (-0.105f, 0.0f).translated (10.0f + i * 25.0f, 1.0f);
            for (auto& r : rs)
            {
                Path p; p.addRoundedRectangle (r.x, r.y, r.w, r.h, 1.2f); p.applyTransform (xf);
                const bool lit = on.containsChar (r.k);
                if (lit) { g.setColour (Colour (0xffff3524).withAlpha (0.25f)); g.strokePath (p, PathStrokeType (3.0f)); }
                g.setColour (lit ? Colour (0xffff3524) : Colour (0xff2a0805));
                g.fillPath (p);
            }
        }
    }
private:
    int num = 1;
};

class LedDot : public Component
{
public:
    void setOn (bool b) { if (b != on) { on = b; repaint(); } }
    void paint (Graphics& g) override { drawLed (g, getLocalBounds().toFloat().reduced (8.0f), on); }
private:
    bool on = false;
};

class TriggerButton : public Component
{
public:
    std::function<void (bool)> onPress;
    void setLit (bool b) { if (b != lit) { lit = b; repaint(); } }
    void paint (Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat().reduced (1.0f);
        const auto c = r.getCentre();
        g.setColour (Colours::black.withAlpha (0.6f)); g.fillEllipse (r.translated (0, 3));
        ColourGradient ring (Colour (0xff3d3d42), c.x, c.y - r.getHeight() * 0.5f, Colour (0xff0f0f10), c.x, c.y + r.getHeight() * 0.5f, false);
        g.setGradientFill (ring); g.fillEllipse (r);
        g.setColour (Colour (0xff0b0b0c)); g.fillEllipse (r.reduced (5));
        g.setColour (Colour (0xff2a2a2d)); g.fillEllipse (r.reduced (7));
        const auto lens = r.reduced (14);
        g.setGradientFill (ColourGradient (col::go.withAlpha (lit ? 0.8f : 0.5f), c.x, c.y, col::go.withAlpha (0.0f), c.x + lens.getWidth(), c.y, true));
        g.fillEllipse (lens.expanded (lit ? 12.0f : 6.0f));
        ColourGradient gl (lit ? Colour (0xfff2fff5) : Colour (0xffc9ffd9), c.x - lens.getWidth() * 0.08f, c.y - lens.getHeight() * 0.15f,
                           lit ? Colour (0xff1d9a4b) : Colour (0xff0f6a30), c.x + lens.getWidth() * 0.5f, c.y, true);
        gl.addColour (0.45, lit ? Colour (0xff7dffa8) : col::go);
        g.setGradientFill (gl); g.fillEllipse (lens);
    }
    void mouseDown (const MouseEvent&) override { if (onPress) onPress (true); }
    void mouseUp (const MouseEvent&) override   { if (onPress) onPress (false); }
private:
    bool lit = false;
};

//==============================================================================
// Preset browser overlay: genres on the left, the genre's 10 presets on the right
class PresetBrowser : public Component
{
public:
    std::function<void (int)> onPick;

    void show (int currentPreset)
    {
        current = currentPreset;
        genre = currentPreset / pk::presetsPerGenre;
        setVisible (true);
        toFront (false);
        repaint();
    }

    void paint (Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        g.setColour (Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (b.translated (0, 12), 12);
        g.setColour (Colour (0xf7080809)); g.fillRoundedRectangle (b, 12);
        g.setColour (Colour (0xff34343a)); g.drawRoundedRectangle (b.reduced (0.5f), 12, 1);
        g.setColour (Colour (0xff26262a)); g.drawVerticalLine (210, 0, b.getBottom());

        for (int i = 0; i < pk::numGenres; ++i)
        {
            const auto r = genreRect (i);
            if (i == genre) { g.setColour (Colour (0xff140807)); g.fillRect (r); }
            g.setFont (label (16.0f, 0.12f));
            if (i == genre) glowText (g, String (pk::genreNames[i]).toUpperCase(), r.withTrimmedLeft (22), Justification::centredLeft, Colour (0xffff4232), 0.2f);
            else { g.setColour (col::inkDim); g.drawText (String (pk::genreNames[i]).toUpperCase(), r.withTrimmedLeft (22), Justification::centredLeft, false); }
            g.setColour (col::inkDim.withAlpha (0.6f)); g.setFont (mono (11.5f));
            g.drawText ("10", r.withTrimmedRight (20), Justification::centredRight, false);
        }

        g.setColour (col::ink); g.setFont (label (17.0f, 0.22f));
        g.drawText (String (pk::genreNames[genre]).toUpperCase(), Rectangle<float> (232, 18, 400, 28), Justification::centredLeft, false);
        const auto xr = closeRect();
        g.setColour (Colour (0xff1d1d20)); g.fillRoundedRectangle (xr, 4);
        g.setColour (col::ink); g.drawLine (xr.getX() + 10, xr.getY() + 10, xr.getRight() - 10, xr.getBottom() - 10, 1.6f);
        g.drawLine (xr.getRight() - 10, xr.getY() + 10, xr.getX() + 10, xr.getBottom() - 10, 1.6f);

        for (int i = 0; i < pk::presetsPerGenre; ++i)
        {
            const int idx = genre * pk::presetsPerGenre + i;
            const auto& p = pk::presets[idx];
            const auto r = presetRect (i);
            const bool on = idx == current, over = idx == hoverIdx;
            g.setColour (on ? Colour (0xff1b150c) : over ? Colour (0xff1a1a1d) : Colour (0xff121214)); g.fillRoundedRectangle (r, 6);
            g.setColour (on ? col::amber : Colour (0xff26262a)); g.drawRoundedRectangle (r.reduced (0.5f), 6, 1);
            g.setColour (Colour (0xffff4232)); g.setFont (mono (15.0f));
            g.drawText (String (i + 1).paddedLeft ('0', 2), Rectangle<float> (r.getX() + 14, r.getY(), 30, r.getHeight()), Justification::centredLeft, false);
            g.setColour (col::ink); g.setFont (label (16.5f, 0.05f));
            g.drawText (p.name, Rectangle<float> (r.getX() + 50, r.getY() + 8, r.getWidth() - 56, 20), Justification::centredLeft, false);
            g.setColour (col::inkDim); g.setFont (mono (11.5f));
            String meta = pk::noteName (p.tune) + " · " + pk::waveNames[p.wave] + " · " + pk::fmtMs (p.dec);
            if (p.sus > 0.0f) meta << " · held";
            g.drawText (meta, Rectangle<float> (r.getX() + 50, r.getY() + 28, r.getWidth() - 56, 14), Justification::centredLeft, false);
        }
    }

    void mouseMove (const MouseEvent& e) override
    {
        int h = -1;
        for (int i = 0; i < pk::presetsPerGenre; ++i)
            if (presetRect (i).contains (e.position)) h = genre * pk::presetsPerGenre + i;
        if (h != hoverIdx) { hoverIdx = h; repaint(); }
    }

    void mouseDown (const MouseEvent& e) override
    {
        if (closeRect().contains (e.position)) { setVisible (false); return; }
        for (int i = 0; i < pk::numGenres; ++i)
            if (genreRect (i).contains (e.position)) { genre = i; repaint(); return; }
        for (int i = 0; i < pk::presetsPerGenre; ++i)
            if (presetRect (i).contains (e.position))
            {
                current = genre * pk::presetsPerGenre + i;
                if (onPick) onPick (current);
                repaint();
                return;
            }
    }

private:
    Rectangle<float> genreRect (int i) const  { return { 0, 16.0f + i * 35.0f, 210, 35 }; }
    Rectangle<float> presetRect (int i) const { return { 230.0f + (i % 2) * 333.0f, 60.0f + (i / 2) * 58.0f, 325, 50 }; }
    Rectangle<float> closeRect() const        { return { (float) getWidth() - 50.0f, 16, 30, 30 }; }

    int genre = 0, current = 0, hoverIdx = -1;
};

//==============================================================================
// The whole 1200 x 760 hardware panel
class PanelContent : public Component
{
public:
    explicit PanelContent (PotatoKickerProcessor& p)
        : proc (p), display (p),
          stereoAttach (*p.apvts.getParameter (pk::id::stereo), [this] (float v) { monoBtn.setToggleState (v < 0.5f, dontSendNotification); stereoBtn.setToggleState (v >= 0.5f, dontSendNotification); }, nullptr)
    {
        auto& s = p.apvts;
        auto knob = [&] (const char* id, const char* title, float size, int x, int y)
        {
            auto* k = knobs.add (new Knob (*s.getParameter (id), title, size));
            addAndMakeVisible (k);
            k->placeAt (x, y);
            return k;
        };

        waveSwitch = std::make_unique<WaveSwitch> (*s.getParameter (pk::id::wave));
        addAndMakeVisible (*waveSwitch);
        waveSwitch->setBounds (15, 64, 152, 140);

        const Colour white (0xffd9dde2);   // white light-pipe for the non-envelope knobs
        knob (pk::id::tune,   "TUNE",       100, 225, 140)->setAccent (white, Colours::white);
        knob (pk::id::pamt,   "PITCH AMT",   92,  95, 292)->setAccent (white, Colours::white);
        knob (pk::id::pdec,   "PITCH TIME",  92, 225, 292)->setAccent (white, Colours::white);
        knob (pk::id::click,  "CLICK",       84,  95, 445)->setAccent (white, Colours::white);
        knob (pk::id::drive,  "DRIVE",       84, 225, 445)->setAccent (white, Colours::white);
        knob (pk::id::thr,    "THRESHOLD",   92, 975, 300)->setAccent (white, Colours::white);
        knob (pk::id::ratio,  "RATIO",       92, 1105, 300)->setAccent (white, Colours::white);
        knob (pk::id::makeup, "MAKEUP",      80, 975, 432)->setAccent (white, Colours::white);
        knob (pk::id::vol,    "VOLUME",      88, 1040, 550)->setAccent (white, Colours::white);
        knob (pk::id::atk,    "ATTACK",     100, 390, 492)->setAccent (Colour (0xff5a1aa8), Colour (0xffb98cff));   // dark purple
        knob (pk::id::dec,    "DECAY",      100, 530, 492)->setAccent (Colour (0xff1550d8), Colour (0xff8cc2ff));   // blue
        knob (pk::id::sus,    "SUSTAIN",    100, 670, 492)->setAccent (Colour (0xffd85a08), Colour (0xffffb46a));   // orange
        knob (pk::id::rel,    "RELEASE",    100, 810, 492)->setAccent (Colour (0xffd8a800), Colour (0xfffff08a));   // yellow

        auto toggle = [&] (const char* id, const char* title, const char* on, const char* off, int x, int y)
        {
            auto* t = toggles.add (new ToggleSwitch (*s.getParameter (id), title, on, off));
            addAndMakeVisible (t);
            t->placeAt (x, y);
        };
        toggle (pk::id::keytrack, "KEY TRACK", "ON", "OFF", 95, 560);
        toggle (pk::id::velocity, "VELOCITY",  "ON", "OFF", 225, 560);
        toggle (pk::id::comp,     "COMP",      "IN", "OUT", 1105, 432);

        addAndMakeVisible (vu);       vu.setBounds (915, 52, 250, 150);
        addAndMakeVisible (display);  display.setBounds (318, 52, 564, 342);

        // strip (y offset 620)
        const int sy = 620;
        for (auto* b : { &gPrev, &gNext, &pPrev, &pNext }) addAndMakeVisible (b);
        gPrev.setBounds (40, sy + 28, 30, 30);  genreWin.setBounds (76, sy + 26, 150, 34); gNext.setBounds (232, sy + 28, 30, 30);
        pPrev.setBounds (290, sy + 31, 30, 30); seg.setBounds (326, sy + 26, 60, 40);      pNext.setBounds (392, sy + 31, 30, 30);
        nameWin.setBounds (458, sy + 26, 270, 34);
        midiLed.setBounds (764, sy + 30, 27, 27);
        monoBtn.setBounds (826, sy + 28, 54, 30); stereoBtn.setBounds (886, sy + 28, 70, 30);
        trigger.setBounds (978, sy + 14, 62, 62);
        for (Component* c : { (Component*) &genreWin, (Component*) &seg, (Component*) &nameWin, (Component*) &midiLed,
                              (Component*) &monoBtn, (Component*) &stereoBtn, (Component*) &trigger })
            addAndMakeVisible (c);
        genreWin.setTitle ("Genre - open preset browser");
        nameWin.setTitle ("Preset name - open preset browser");
        monoBtn.setTitle ("Mono output");
        stereoBtn.setTitle ("Stereo output");
        trigger.setTitle ("Trigger C1");

        gPrev.onClick = [this] { pick (((current() / 10) + 9) % 10 * 10, true); };
        gNext.onClick = [this] { pick (((current() / 10) + 1) % 10 * 10, true); };
        pPrev.onClick = [this] { pick (current() / 10 * 10 + (current() % 10 + 9) % 10, true); };
        pNext.onClick = [this] { pick (current() / 10 * 10 + (current() % 10 + 1) % 10, true); };
        genreWin.onClick = [this] { browser.show (current()); };
        nameWin.onClick  = [this] { browser.show (current()); };
        monoBtn.onClick   = [this] { stereoAttach.setValueAsCompleteGesture (0.0f); };
        stereoBtn.onClick = [this] { stereoAttach.setValueAsCompleteGesture (1.0f); };
        stereoAttach.sendInitialUpdate();

        trigger.onPress = [this] (bool down)
        {
            if (down) proc.keyboardState.noteOn (1, 36, 110.0f / 127.0f);
            else      proc.keyboardState.noteOff (1, 36, 0.0f);
        };

        addChildComponent (browser);
        browser.setBounds (150, 46, 900, 520);
        browser.onPick = [this] (int i) { pick (i, true); };

        refresh();
    }

    void refresh()
    {
        const int c = current();
        genreWin.setText (String (pk::genreNames[c / 10]).toUpperCase());
        seg.setNumber (c % 10 + 1);
        nameWin.setText (String (pk::presets[c].name) + (proc.presetIsModified() ? String (CharPointer_UTF8 (" \xe2\x80\xa2")) : String()));
        if (proc.noteOnCounter.load() != lastCounter) { lastCounter = proc.noteOnCounter.load(); lastNoteSeen = Time::getMillisecondCounterHiRes(); }
        const bool active = proc.heldNotes.load() > 0 || Time::getMillisecondCounterHiRes() - lastNoteSeen < 90.0;
        midiLed.setOn (active);
        trigger.setLit (active);
        vu.setLevel (proc.meterRms.load());
        display.tick();
    }

    void paint (Graphics& g) override
    {
        g.fillAll (col::page);
        drawPlate (g, { 0, 0, 1200, 612 });
        drawPlate (g, { 0, 620, 1200, 140 });
        for (auto pnt : { Point<float> (18, 18), Point<float> (1182, 18), Point<float> (18, 594), Point<float> (1182, 594),
                          Point<float> (18, 638), Point<float> (1182, 638), Point<float> (18, 742), Point<float> (1182, 742) })
            drawScrew (g, pnt);

        g.setColour (col::ink); g.setFont (label (23.0f, 0.42f));
        g.drawText ("POTATO KICKER", Rectangle<float> (0, 12, 1200, 30), Justification::centred, false);
        g.setColour (col::inkDim); g.setFont (label (15.5f, 0.32f));
        g.drawText ("KICK & BASS GENERATOR", Rectangle<float> (0, 578, 1200, 20), Justification::centred, false);

        section (g, "OSCILLATOR", 30, 46, 260);
        section (g, "PITCH ENVELOPE", 30, 212, 260);
        section (g, "CHARACTER", 30, 362, 260);
        section (g, "COMPRESSOR", 910, 214, 260);
        section (g, "AMP ENVELOPE", 330, 408, 540);

        // strip captions
        g.setColour (col::ink); g.setFont (label (13.0f, 0.16f));
        auto cap = [&] (const String& s, float cx, float y) { g.drawText (s, Rectangle<float> (cx - 120, y, 240, 16), Justification::centred, false); };
        cap ("GENRE", 151, 688);
        cap ("PRESET", 356, 694);
        cap ("CLICK NAME TO BROWSE ALL 100", 593, 688);
        cap ("MIDI IN", 777, 688);
        cap ("OUTPUT", 891, 688);
        cap ("TRIGGER", 1009, 704);
        g.setFont (label (22.0f, 0.3f, true));
        g.drawText ("VST3", Rectangle<float> (1050, 652, 118, 26), Justification::centredRight, false);
        g.setColour (col::inkDim); g.setFont (mono (12.0f));
        g.drawText (CharPointer_UTF8 ("WIN 11 \xc2\xb7 macOS"), Rectangle<float> (1030, 680, 138, 14), Justification::centredRight, false);
        g.drawText (CharPointer_UTF8 ("100 PRESETS"), Rectangle<float> (1050, 695, 118, 14), Justification::centredRight, false);
    }

private:
    int current() const { return proc.currentPreset.load(); }

    void pick (int index, bool audition)
    {
        proc.loadPreset (index);
        refresh();
        if (audition)
        {
            proc.keyboardState.noteOn (1, 36, 110.0f / 127.0f);
            const int holdMs = pk::presets[index].sus >= 5.0f ? 600 : 60;
            Component::SafePointer<PanelContent> safe (this);
            Timer::callAfterDelay (holdMs, [safe] { if (safe != nullptr) safe->proc.keyboardState.noteOff (1, 36, 0.0f); });
        }
    }

    static void drawPlate (Graphics& g, Rectangle<float> r)
    {
        ColourGradient grad (Colour (0xff1b1b1e), 0, r.getY(), Colour (0xff0e0e10), 0, r.getBottom(), false);
        grad.addColour (0.45, Colour (0xff141416));
        g.setColour (Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (r.translated (0, 6), 6);
        g.setGradientFill (grad); g.fillRoundedRectangle (r, 6);
        {
            Graphics::ScopedSaveState s (g);
            g.reduceClipRegion (r.toNearestInt());
            g.setColour (Colours::white.withAlpha (0.018f));
            for (float y = r.getY(); y < r.getBottom(); y += 3.0f) g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
        }
        g.setColour (Colours::white.withAlpha (0.08f)); g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + 4, r.getRight() - 4);
        g.setColour (Colours::black.withAlpha (0.6f)); g.drawHorizontalLine ((int) r.getBottom() - 2, r.getX() + 4, r.getRight() - 4);
    }

    static void drawScrew (Graphics& g, Point<float> c)
    {
        const auto r = Rectangle<float> (13, 13).withCentre (c);
        g.setGradientFill (ColourGradient (Colour (0xff6b6b70), c.x - 2, c.y - 3, Colour (0xff1a1a1c), c.x + 6, c.y + 6, true));
        g.fillEllipse (r);
        g.setColour (Colours::black); g.drawEllipse (r, 1.0f);
        const auto a = polar (c, 4.5f, 125.0f), b = polar (c, 4.5f, -55.0f);
        g.setColour (Colour (0xff0a0a0a)); g.drawLine (a.x, a.y, b.x, b.y, 2.0f);
    }

    static void section (Graphics& g, const String& text, float x, float y, float w)
    {
        const auto f = label (12.5f, 0.24f);
        const float tw = GlyphArrangement::getStringWidth (f, text) + 20.0f;
        const float cy = y + 7.0f, mid = x + w * 0.5f;
        g.setGradientFill (ColourGradient (Colour (0x003a3a3f), x, cy, Colour (0xff3a3a3f), mid - tw * 0.5f, cy, false));
        g.drawHorizontalLine ((int) cy, x, mid - tw * 0.5f);
        g.setGradientFill (ColourGradient (Colour (0xff3a3a3f), mid + tw * 0.5f, cy, Colour (0x003a3a3f), x + w, cy, false));
        g.drawHorizontalLine ((int) cy, mid + tw * 0.5f, x + w);
        g.setColour (col::inkDim); g.setFont (f);
        g.drawText (text, Rectangle<float> (mid - tw * 0.5f, y, tw, 15), Justification::centred, false);
    }

    PotatoKickerProcessor& proc;
    OwnedArray<Knob> knobs;
    OwnedArray<ToggleSwitch> toggles;
    std::unique_ptr<WaveSwitch> waveSwitch;
    VuMeter vu;
    EnvDisplay display;
    SquareButton gPrev { SquareButton::arrowLeft, "Previous genre" }, gNext { SquareButton::arrowRight, "Next genre" },
                 pPrev { SquareButton::arrowLeft, "Previous preset" }, pNext { SquareButton::arrowRight, "Next preset" },
                 monoBtn { SquareButton::text, "MONO" }, stereoBtn { SquareButton::text, "STEREO" };
    LedWindow genreWin { true }, nameWin { false };
    SevenSeg seg;
    LedDot midiLed;
    TriggerButton trigger;
    PresetBrowser browser;
    ParameterAttachment stereoAttach;
    uint32_t lastCounter = 0;
    double lastNoteSeen = 0.0;
};

//==============================================================================
PotatoKickerEditor::PotatoKickerEditor (PotatoKickerProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    panel = std::make_unique<PanelContent> (p);
    addAndMakeVisible (*panel);
    panel->setBounds (0, 0, panelW, panelH);

    setResizable (true, true);
    if (auto* c = getConstrainer())
    {
        c->setFixedAspectRatio ((double) panelW / panelH);
        c->setSizeLimits (panelW * 6 / 10, panelH * 6 / 10, panelW * 3 / 2, panelH * 3 / 2);
    }
    setSize (panelW * 9 / 10, panelH * 9 / 10);
    startTimerHz (30);
}

PotatoKickerEditor::~PotatoKickerEditor() { stopTimer(); }

void PotatoKickerEditor::paint (Graphics& g) { g.fillAll (col::page); }

void PotatoKickerEditor::resized()
{
    panel->setTransform (AffineTransform::scale ((float) getWidth() / (float) panelW));
}

void PotatoKickerEditor::timerCallback() { panel->refresh(); }
