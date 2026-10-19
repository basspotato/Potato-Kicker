#include "PluginEditor.h"
#include <BinaryData.h>

using namespace juce;

//==============================================================================
// Palette and fonts
namespace col
{
    // HUD palette: near-black panels with glowing orange accents
    const Colour page    { 0xff070707 };
    const Colour ink     { 0xffffb35e };   // labels
    const Colour inkDim  { 0xff9a7658 };   // values and secondary text
    const Colour amber   { 0xffff8a1e };   // primary accent
    const Colour amberHi { 0xffffc27a };   // hot highlight
    const Colour led     { 0xffff7a12 };
    const Colour ledOff  { 0xff2c1406 };
    const Colour go      { 0xffff8a1e };
    const Colour cream   { 0xffffa040 };   // lit buttons and tabs
    const Colour tick    { 0xffb3a493 };
    const Colour faceInk { 0xffff9a3c };
    const Colour faceRed { 0xffff3d1a };
    const Colour line    { 0xffff8a1e };   // tile borders and brackets (used with low alpha)
    const Colour orange  { 0xffff7212 };   // start of the orange -> yellow light ramp
    const Colour yellow  { 0xffffd23f };   // end of the ramp
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
            ColourGradient gr (Colour (0xfffff3c8), c.x - r.getWidth() * 0.12f, c.y - r.getHeight() * 0.15f, Colour (0xffa84300), c.x + r.getWidth() * 0.5f, c.y, true);
            gr.addColour (0.45, col::led);
            g.setGradientFill (gr);
        }
        else
        {
            g.setGradientFill (ColourGradient (Colour (0xff4a2208), c.x - r.getWidth() * 0.1f, c.y - r.getHeight() * 0.15f, col::ledOff, c.x + r.getWidth() * 0.5f, c.y, true));
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
        g.setColour (col::line.withAlpha (0.35f)); g.fillRoundedRectangle (r.expanded (1.0f), 4.0f);
        g.setColour (Colour (0xff080504)); g.fillRoundedRectangle (r, 3.0f);
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
            g.setColour (Colour (0xffd9c4ad));
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
    // Orange -> yellow light ramp used across the whole console (0 = orange, 1 = yellow)
    Colour ramp (float t) { return col::orange.interpolatedWith (col::yellow, jlimit (0.0f, 1.0f, t)); }

    // Glowing text filled with the orange -> yellow ramp from left to right
    void glowTextRamp (Graphics& g, const String& s, Rectangle<float> r, Justification j, float glow)
    {
        g.setColour (col::orange.withAlpha (glow));
        for (auto d : { Point<float> (-1, 0), Point<float> (1, 0), Point<float> (0, -1), Point<float> (0, 1) })
            g.drawText (s, r.translated (d.x, d.y), j, false);
        g.setGradientFill (ColourGradient (col::orange, r.getX(), 0, col::yellow, r.getRight(), 0, false));
        g.drawText (s, r, j, false);
    }

    // Strokes the knob-travel arc between t0 and t1 (0..1 of the 270 degree sweep) in short segments,
    // each coloured from the ramp, so the light fades from orange at the start to yellow at the end
    void strokeRampArc (Graphics& g, Point<float> c, float r, float t0, float t1, float width, float alpha, float bright = 0.0f)
    {
        const int n = jmax (2, (int) ((t1 - t0) * 72.0f));
        for (int i = 0; i < n; ++i)
        {
            const float a = t0 + (t1 - t0) * (float) i / (float) n, b = t0 + (t1 - t0) * (float) (i + 1) / (float) n;
            Path seg; seg.addCentredArc (c.x, c.y, r, r, 0.0f, degToRad (-135.0f + 270.0f * a), degToRad (-135.0f + 270.0f * b), true);
            g.setColour (ramp (0.5f * (a + b)).brighter (bright).withAlpha (jlimit (0.0f, 1.0f, alpha)));
            g.strokePath (seg, PathStrokeType (width, PathStrokeType::curved, PathStrokeType::butt));
        }
    }

    // Full ring lit with the ramp: orange at the bottom, yellow at the top
    void strokeRampRing (Graphics& g, Point<float> c, float r, float width, float alpha)
    {
        for (int i = 0; i < 72; ++i)
        {
            const float a = i * 5.0f, b = a + 5.0f;
            Path seg; seg.addCentredArc (c.x, c.y, r, r, 0.0f, degToRad (a), degToRad (b), true);
            g.setColour (ramp (0.5f * (1.0f + std::cos (degToRad (a + 2.5f)))).withAlpha (alpha));
            g.strokePath (seg, PathStrokeType (width, PathStrokeType::curved, PathStrokeType::butt));
        }
    }

    // Knob: black glossy body with a backlit light-pipe ring. The lit part of the ring and the scale fade from
    // orange at the start of travel to yellow at the end, and the whole light gets brighter as the knob turns up.
    void drawLitKnob (Graphics& g, Point<float> c, float size, float norm, bool numbered = true, bool ticks = true)
    {
        const float n = jlimit (0.0f, 1.0f, norm);
        const float k = 0.3f + 0.7f * n;                         // light intensity
        const float rO = size * 0.5f - 1.5f, rB = size * 0.25f;
        const float rw = size * 0.036f;
        const float rR = rB + rw * 0.5f + size * 0.04f;
        const auto box = [&] (float r) { return Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c); };

        for (int i = 0; ticks && i <= 40; ++i)
        {
            const bool major = i % 4 == 0;
            const float t = (float) i / 40.0f;
            const bool on = t <= n + 0.0001f && n > 0.001f;
            const auto p1 = polar (c, major ? rO - 5.5f : rO - 3.0f, -135.0f + 270.0f * t), p2 = polar (c, rO, -135.0f + 270.0f * t);
            if (on)
            {
                g.setColour (ramp (t).withAlpha (0.35f * k)); g.drawLine (p1.x, p1.y, p2.x, p2.y, major ? 4.0f : 3.0f);
                g.setColour (ramp (t).brighter (0.35f));      g.drawLine (p1.x, p1.y, p2.x, p2.y, major ? 1.5f : 1.0f);
            }
            else
            {
                g.setColour (col::tick.withAlpha (major ? 0.75f : 0.4f)); g.drawLine (p1.x, p1.y, p2.x, p2.y, major ? 1.3f : 0.8f);
            }
        }

        const float rn = 0.5f * ((rR + rw * 0.5f + 1.2f) + (rO - 5.5f));
        g.setFont (label (size * 0.1f));
        for (int j = 0; numbered && j <= 10; ++j)
        {
            const bool on = j / 10.0f <= n + 0.0001f && n > 0.001f;
            g.setColour (on ? ramp (j / 10.0f).brighter (0.55f) : Colour (0xff8f8174));
            g.drawText (String (j), Rectangle<float> (14.0f, 11.0f).withCentre (polar (c, rn, -135.0f + j * 27.0f)), Justification::centred, false);
        }

        // groove, dim backlit ring, then the lit value arc with bloom
        g.setColour (Colour (0xff040303)); g.drawEllipse (box (rR), rw + 3.0f);
        strokeRampArc (g, c, rR, 0.0f, 1.0f, rw * 0.8f, 0.10f + 0.12f * k);
        if (n > 0.001f)
        {
            strokeRampArc (g, c, rR, 0.0f, n, rw * 4.0f, 0.12f * k);
            strokeRampArc (g, c, rR, 0.0f, n, rw * 2.3f, 0.28f * k);
            strokeRampArc (g, c, rR, 0.0f, n, rw * 1.1f, 0.65f + 0.35f * k);
            strokeRampArc (g, c, rR, 0.0f, n, rw * 0.6f, 1.0f, 0.45f);
            const auto tip = polar (c, rR, -135.0f + 270.0f * n);
            g.setColour (ramp (n).brighter (0.6f).withAlpha (0.35f)); g.fillEllipse (Rectangle<float> (rw * 3.2f, rw * 3.2f).withCentre (tip));
            g.setColour (Colour (0xfffff4d6));                     g.fillEllipse (Rectangle<float> (rw * 1.3f, rw * 1.3f).withCentre (tip));
        }

        // light spilling onto the skirt
        g.setColour (ramp (n).withAlpha (0.12f + 0.3f * k)); g.drawEllipse (box (rB + 2.2f), 1.6f);

        // skirt, glossy body, shine
        g.setGradientFill (ColourGradient (Colour (0xff3a3632), c.x, c.y - rB - 3, Colours::black, c.x, c.y + rB + 3, false));
        g.fillEllipse (box (rB + 1.6f));
        ColourGradient body (Colour (0xff4e4a46), c.x - rB * 0.2f, c.y - rB * 0.4f, Colour (0xff020202), c.x - rB * 0.2f + rB * 1.56f, c.y - rB * 0.4f, true);
        body.addColour (0.32, Colour (0xff1d1b1a));
        g.setGradientFill (body); g.fillEllipse (box (rB - 1.0f));
        g.setColour (Colours::black); g.drawEllipse (box (rB - 1.0f), 0.8f);
        const Point<float> sc (c.x - rB * 0.22f, c.y - rB * 0.4f);
        g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.24f), sc.x, sc.y, Colours::white.withAlpha (0.0f), sc.x + rB * 0.55f, sc.y, true));
        g.fillEllipse (Rectangle<float> (rB * 1.1f, rB * 0.6f).withCentre (sc));

        // pointer: lit line in the ramp colour for the current position
        const auto p1 = polar (c, rB * 0.15f, -135.0f + 270.0f * n), p2 = polar (c, rB - 3.0f, -135.0f + 270.0f * n);
        g.setColour (ramp (n).withAlpha (0.25f + 0.4f * k)); g.drawLine (p1.x, p1.y, p2.x, p2.y, 5.0f);
        g.setColour (Colour (0xfffff1d8));                   g.drawLine (p1.x, p1.y, p2.x, p2.y, jmax (2.0f, size * 0.026f));
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
        g.setFont (label (16.0f, 0.1f));
        glowText (g, name, Rectangle<float> (0, 0, w, 17), Justification::centred, col::ink, 0.22f);
        drawLitKnob (g, { w * 0.5f, 18.0f + size * 0.5f }, size, param.getValue(), size >= 84.0f);
        const Colour valueCol = ramp (param.getValue()).brighter (0.4f);
        g.setColour (dragging ? valueCol : Colour (0xffb98c62)); g.setFont (mono (12.5f));
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
// Seven-position rotary waveform switch with LEDs
class WaveSwitch : public Component
{
public:
    static constexpr int numWaves = 7;

    explicit WaveSwitch (RangedAudioParameter& p)
        : param (p), attach (p, [this] (float v) { index = jlimit (0, numWaves - 1, (int) v); repaint(); }, nullptr)
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
        for (int i = 0; i < numWaves; ++i)
        {
            drawLed (g, Rectangle<float> (8, 8).withCentre (polar (c, 40, angles[i])), i == index);
            g.setFont (label (12.5f, 0.04f));
            if (i == index) glowText (g, pk::waveNames[i], Rectangle<float> (44, 14).withCentre (polar (c, 55, angles[i])), Justification::centred, col::yellow, 0.3f);
            else { g.setColour (Colour (0xffa47a55)); g.drawText (pk::waveNames[i], Rectangle<float> (44, 14).withCentre (polar (c, 55, angles[i])), Justification::centred, false); }
        }
        drawLitKnob (g, c, 70.0f, (angles[index] + 135.0f) / 270.0f, false, false);
        g.setFont (label (15.0f, 0.12f));
        glowText (g, "WAVEFORM", Rectangle<float> (130, 17).withCentre ({ c.x, c.y + 51 }), Justification::centred, col::ink, 0.22f);
    }

    void mouseDown (const MouseEvent& e) override
    {
        const auto c = centre();
        int target = (index + 1) % numWaves;
        float best = 14.0f;
        for (int i = 0; i < numWaves; ++i)
        {
            const float d = e.position.getDistanceFrom (polar (c, 54, angles[i]));
            if (d < best) { best = d; target = i; }
        }
        attach.setValueAsCompleteGesture ((float) target);
    }

private:
    RangedAudioParameter& param;
    ParameterAttachment attach;
    int index = 0;
    const float angles[numWaves] { -120.0f, -80.0f, -40.0f, 0.0f, 40.0f, 80.0f, 120.0f };
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
        g.setColour (col::inkDim); g.setFont (label (13.0f, 0.08f));
        g.drawText (onT,  Rectangle<float> (0, c.y - 7, 30, 14), Justification::centredRight, false);
        g.drawText (offT, Rectangle<float> (70, c.y - 7, 30, 14), Justification::centredLeft, false);
        g.setColour (Colour (0xff0b0908)); g.fillEllipse (Rectangle<float> (30, 30).withCentre (c));
        g.setColour (col::line.withAlpha (0.35f)); g.drawEllipse (Rectangle<float> (30, 30).withCentre (c), 1.0f);
        g.setGradientFill (ColourGradient (Colour (0xff5d544c), c.x - 12, c.y - 12, Colour (0xff17130f), c.x + 12, c.y + 12, false));
        g.fillEllipse (Rectangle<float> (24, 24).withCentre (c));
        g.setColour (Colour (0xff2a2a2d)); g.fillEllipse (Rectangle<float> (12, 12).withCentre (c));
        const auto tip = polar (c, 15.0f, on ? -32.0f : 32.0f);
        g.setColour (Colour (0xffcfc6bd)); g.drawLine (c.x, c.y, tip.x, tip.y, 5.0f);
        g.setColour (on ? col::amberHi : Colour (0xffe2dad2)); g.fillEllipse (Rectangle<float> (7.2f, 7.2f).withCentre (tip));
        g.setFont (label (15.0f, 0.12f));
        glowText (g, name, Rectangle<float> (0, 60, 100, 17), Justification::centred, col::ink, 0.22f);
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
        g.setColour (Colour (0xff060504)); g.fillRoundedRectangle (1, 1, 248, 148, 12);
        g.setColour (col::line.withAlpha (0.35f)); g.drawRoundedRectangle (1, 1, 248, 148, 12, 1.2f);
        const Rectangle<float> face (13, 13, 224, 102);
        {
            Graphics::ScopedSaveState s (g);
            Path clip; clip.addRoundedRectangle (face, 7.0f);
            g.reduceClipRegion (clip);
            g.setGradientFill (ColourGradient (Colour (0xff160d06), 0, face.getY(), Colour (0xff050302), 0, face.getBottom(), false));
            g.fillRect (face);
            g.setGradientFill (ColourGradient (Colour (0x38ff8a1e), 125, 122, Colour (0x00ff8a1e), 125 + 150, 122, true));
            g.fillRect (face);
            g.setColour (Colour (0x10ff8a1e));
            for (float x = face.getX() + 8; x < face.getRight(); x += 12) g.drawVerticalLine ((int) x, face.getY(), face.getBottom());
            for (float y = face.getY() + 6; y < face.getBottom(); y += 12) g.drawHorizontalLine ((int) y, face.getX(), face.getRight());

            Path arc;  arc.addCentredArc (pivot.x, pivot.y, R, R, 0, degToRad (ang (-20)), degToRad (ang (0)), true);
            g.setGradientFill (ColourGradient (col::orange, 20, 0, col::yellow, 230, 0, false)); g.strokePath (arc, PathStrokeType (1.6f));
            Path red;  red.addCentredArc (pivot.x, pivot.y, R, R, 0, degToRad (ang (0)), degToRad (ang (3)), true);
            g.setColour (col::faceRed); g.strokePath (red, PathStrokeType (4.0f));
            g.setFont (label (11.5f));
            for (int v : { -20, -10, -7, -5, -3, -2, -1, 0, 1, 2, 3 })
            {
                const float a = ang ((float) v);
                const auto p1 = polar (pivot, R - 9, a), p2 = polar (pivot, R, a), pt = polar (pivot, R + 11, a);
                g.setColour (v > 0 ? col::faceRed : ramp ((v + 20.0f) / 20.0f));
                g.drawLine (p1.x, p1.y, p2.x, p2.y, 1.4f);
                g.drawText (v > 0 ? "+" + String (v) : String (v), Rectangle<float> (24, 12).withCentre (pt), Justification::centred, false);
            }
            g.setFont (label (13.5f, 0.0f, true));
            g.setColour (col::faceInk); g.drawText ("VU", Rectangle<float> (30, 22, 30, 14), Justification::left, false);
            g.setColour (col::faceRed); g.drawText ("VU", Rectangle<float> (190, 22, 30, 14), Justification::right, false);
            g.setColour (col::amber.withAlpha (0.45f)); g.setFont (label (9.0f, 0.25f));
            g.drawText ("POTATO KICKER", Rectangle<float> (60, 90, 130, 12), Justification::centred, false);
            const auto tip = polar (pivot, R + 4, ang (jlimit (-20.0f, 3.5f, level)));
            g.setColour (col::amber.withAlpha (0.30f)); g.drawLine (pivot.x, pivot.y, tip.x, tip.y, 5.0f);
            g.setColour (col::yellow.brighter (0.3f)); g.drawLine (pivot.x, pivot.y, tip.x, tip.y, 1.6f);
        }
        g.setFont (label (14.5f, 0.3f));
        glowText (g, "OUTPUT", Rectangle<float> (60, 125, 130, 17), Justification::centred, col::ink, 0.22f);
        drawLed (g, Rectangle<float> (11, 11).withCentre ({ 236.5f, 126.5f }), level > 0.0f);
    }

public:
    float getLevel() const noexcept { return level; }

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
        const auto bb = getLocalBounds().toFloat();
        g.setGradientFill (ColourGradient (Colour (0xff100d0a), 0, 0, Colour (0xff060504), 0, bb.getHeight(), false));
        g.fillRoundedRectangle (bb, 10.0f);
        g.setColour (col::line.withAlpha (0.30f)); g.drawRoundedRectangle (bb.reduced (0.5f), 10.0f, 1.0f);
        g.setColour (col::line.withAlpha (0.85f));
        for (auto [x, y, dx, dy] : { std::tuple<float, float, float, float> { 4, 4, 1, 1 }, { bb.getRight() - 4, 4, -1, 1 },
                                     { 4, bb.getBottom() - 4, 1, -1 }, { bb.getRight() - 4, bb.getBottom() - 4, -1, -1 } })
        {
            g.fillRect (dx > 0 ? x : x - 16, dy > 0 ? y : y - 2, 16.0f, 2.0f);
            g.fillRect (dx > 0 ? x : x - 2, dy > 0 ? y : y - 16, 2.0f, 16.0f);
        }

        // tabs
        const char* names[4] { "AMP", "PITCH", "COMP", "WAVE" };
        for (int i = 0; i < 4; ++i)
        {
            const auto r = tabRect (i);
            if (i == view)
            {
                g.setColour (col::amber.withAlpha (0.28f)); g.fillRoundedRectangle (r.expanded (3), 5);
                g.setGradientFill (ColourGradient (col::yellow.brighter (0.2f), 0, r.getY(), col::orange, 0, r.getBottom(), false));
                g.fillRoundedRectangle (r, 3);
                g.setColour (Colour (0xff1a0b02));
            }
            else
            {
                g.setGradientFill (ColourGradient (Colour (0xff1d1713), 0, r.getY(), Colour (0xff0e0b09), 0, r.getBottom(), false));
                g.fillRoundedRectangle (r, 3);
                g.setColour (col::line.withAlpha (0.4f)); g.drawRoundedRectangle (r.reduced (0.5f), 3, 1);
                g.setColour (Colour (0xffc0824e));
            }
            g.setFont (label (14.5f, 0.12f));
            g.drawText (names[i], r, Justification::centred, false);
        }
        g.setFont (mono (14.0f));
        glowText (g, pk::noteName (last.tune) + " · " + String (last.tune, 1) + " Hz · " + pk::waveNames[last.wave],
                  Rectangle<float> (300, 12, 242, 22), Justification::centredRight, col::yellow, 0.22f);

        // screen
        {
            Graphics::ScopedSaveState s (g);
            g.setOrigin (scr.getPosition());
            g.reduceClipRegion (Rectangle<int> (0, 0, CW, CH));
            drawScreen (g);
        }
        g.setColour (col::line.withAlpha (0.25f)); g.drawRoundedRectangle (scr.toFloat(), 7.0f, 1.0f);

        // footer
        const char* foot[4] { "Drag ATTACK, DECAY/SUSTAIN and RELEASE points", "Drag START, PITCH TIME and TUNE points", "Drag THRESHOLD/MAKEUP and RATIO points", "Live output, last 2.4 seconds" };
        g.setColour (Colour (0xffc48a4c)); g.setFont (mono (12.5f));
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

        // waveform: deep orange at the centre line rising to bright yellow at the peaks, with a bloom
        if (view < 2 && ! preview.empty())
        {
            const int N = (int) preview.size();
            const bool mainView = view == 0;
            const float cy = ay (0.0f);
            float peak = 0.05f;   // scale the gradient to this sound's own peak so every waveform spans orange -> yellow
            for (auto v : preview) peak = jmax (peak, std::abs (v));
            ColourGradient upG (Colour (0xffff5a00), 0, cy, Colour (0xfffff3b0), 0, ay (peak), false);
            upG.addColour (0.30, Colour (0xffff9a1a));
            upG.addColour (0.65, Colour (0xffffd23f));
            ColourGradient loG (upG); loG.point2 = { 0, ay (-peak) };
            Path upper, lower;
            for (int c = 0; c < GW; ++c)
            {
                const int i0 = (int) ((float) c / GW * N), i1 = jmax (i0 + 1, (int) ((float) (c + 1) / GW * N));
                float mn = 1.0f, mx = -1.0f;
                for (int i = i0; i < i1 && i < N; ++i) { mn = jmin (mn, preview[(size_t) i]); mx = jmax (mx, preview[(size_t) i]); }
                if (mx < mn) continue;
                const float top = ay (mx), bot = ay (mn) + 0.6f, x = (float) (PL + c);
                if (mainView)
                {
                    g.setColour (Colour (0xffff7a00).withAlpha (0.20f)); g.fillRect (x - 2.5f, top - 4.0f, 6.0f, bot - top + 8.0f);
                    // each column glows from deep orange at the centre line to yellow at its own tip
                    if (cy - top > 0.5f) { auto gu = upG; gu.point2 = { 0, top }; g.setGradientFill (gu); g.fillRect (x, top, 1.0f, cy - top); }
                    if (bot - cy > 0.5f) { auto gl = loG; gl.point2 = { 0, bot }; g.setGradientFill (gl); g.fillRect (x, cy, 1.0f, bot - cy); }
                    if (upper.isEmpty()) { upper.startNewSubPath (x, top); lower.startNewSubPath (x, bot); }
                    else                 { upper.lineTo (x, top);          lower.lineTo (x, bot); }
                }
                else
                {
                    g.setColour (ramp (jmax (mx, -mn)).withAlpha (0.45f)); g.drawVerticalLine (PL + c, top, bot);
                }
            }
            if (mainView)
            {
                g.setColour (col::yellow.withAlpha (0.35f)); g.strokePath (upper, PathStrokeType (3.0f)); g.strokePath (lower, PathStrokeType (3.0f));
                g.setColour (Colour (0xfffff2c4));           g.strokePath (upper, PathStrokeType (1.1f)); g.strokePath (lower, PathStrokeType (1.1f));
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
            g.setColour (col::yellow.withAlpha (0.25f)); g.strokePath (env, PathStrokeType (5.0f));
            g.setColour (Colour (0xfffff3d0)); g.strokePath (env, PathStrokeType (1.8f));
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
            g.setColour (col::orange.withAlpha (0.3f)); g.strokePath (curve, PathStrokeType (6.0f));
            g.setGradientFill (ColourGradient (col::yellow, (float) PL, 0, col::orange, (float) (PL + GW), 0, false));
            g.strokePath (curve, PathStrokeType (2.2f));
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
            g.setColour (col::orange.withAlpha (0.3f * a)); g.strokePath (curve, PathStrokeType (6.0f));
            auto cg = ColourGradient (col::orange.withAlpha (a), (float) PL, 0, col::yellow.withAlpha (a), (float) (PL + GW), 0, false);
            g.setGradientFill (cg); g.strokePath (curve, PathStrokeType (2.2f));
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

        // ember red in the sub, through orange and amber, to bright and pale yellow in the highs
        static const Colour bands[6] { Colour (0xffd02800), Colour (0xffff5a00), Colour (0xffff8a12),
                                       Colour (0xffffb028), Colour (0xffffd84a), Colour (0xfffff4b0) };
        static const Colour kickCol (0xff7a1500), edgeCol (0xffffe066);
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
            const float body = jmax (1.0f, heightOf (rms * 2.2f));
            const float halo = jmax (body, heightOf (wPeak));

            ColourGradient grad;
            grad.isRadial = false;
            float acc = 0.0f;
            bool first = true;
            for (int k = 5; k >= 0; --k)
            {
                const float sh = waveShare[(size_t) k];
                auto col = bands[k].brighter (0.2f);
                if (k <= 1) col = col.interpolatedWith (kickCol, jlimit (0.0f, 0.75f, waveKickSm * 0.75f));
                const float p = jlimit (0.0f, 1.0f, acc + sh * 0.5f);
                if (first) { grad.addColour (0.0, col.brighter (0.5f)); first = false; }
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
                    gi.setGradientFill (ColourGradient (edgeCol.withAlpha (0.6f), xc, cy + side * body, edgeCol.withAlpha (0.0f), xc, cy + side * halo, false));
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
        g.setGradientFill (ColourGradient (Colour (0xff120c07), 0, 0, Colour (0xff040302), 0, (float) CH, false));
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

        if (waveImg.isValid())
        {
            g.drawImageAt (waveImg, PL, PT);
            g.setOpacity (0.35f);                       // soft bloom around the bright waveform
            g.drawImageAt (waveImg, PL - 1, PT);
            g.drawImageAt (waveImg, PL + 1, PT);
            g.drawImageAt (waveImg, PL, PT - 1);
            g.drawImageAt (waveImg, PL, PT + 1);
            g.setOpacity (1.0f);
        }

        // glowing horizon: white-hot core fading through amber to blue at the edges
        ColourGradient hz (col::orange.withAlpha (0.0f), (float) PL, cy, col::yellow.withAlpha (0.0f), (float) (PL + GW), cy, false);
        hz.addColour (0.20, col::orange.withAlpha (0.25f));
        hz.addColour (0.50, Colour (0xfffff0b8).withAlpha (0.45f));
        hz.addColour (0.80, col::yellow.withAlpha (0.28f));
        g.setGradientFill (hz); g.fillRect ((float) PL, cy - 3.0f, (float) GW, 7.0f);
        hz.multiplyOpacity (1.8f);
        g.setGradientFill (hz); g.fillRect ((float) PL, cy, (float) GW, 1.0f);

        // "now" line at the right edge
        const float x = (float) (PL + GW) - 1.0f;
        g.setColour (col::yellow.withAlpha (0.25f)); g.fillRect (x - 3.0f, (float) PT, 6.0f, (float) GH);
        g.setColour (col::yellow.brighter (0.4f));    g.fillRect (x, (float) PT, 1.0f, (float) GH);
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
        Colour ink;
        if (getToggleState())
        {
            g.setColour (col::amber.withAlpha (0.28f)); g.fillRoundedRectangle (r.expanded (3), 5);
            g.setGradientFill (ColourGradient (col::yellow.brighter (0.2f), 0, r.getY(), col::orange, 0, r.getBottom(), false));
            g.fillRoundedRectangle (r, 3);
            ink = Colour (0xff1a0b02);
        }
        else
        {
            g.setGradientFill (ColourGradient (Colour (over ? 0xff261d16 : 0xff1d1713), 0, r.getY(), Colour (0xff0e0b09), 0, r.getBottom(), false));
            g.fillRoundedRectangle (r, 3);
            g.setColour (col::line.withAlpha (over ? 0.75f : 0.45f)); g.drawRoundedRectangle (r.reduced (0.5f), 3, 1);
            ink = col::ink;
        }
        g.setColour (ink);
        if (kind == text)
        {
            g.setFont (label (14.0f, 0.06f, true));
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
            g.setFont (mono (19.0f));
            glowTextRamp (g, txt, r.reduced (14.0f, 0.0f), Justification::centred, 0.35f);
        }
        else
        {
            g.setFont (mono (18.0f));
            glowText (g, txt, r.withTrimmedLeft (12).withTrimmedRight (10), Justification::centredLeft, col::yellow, 0.25f);
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
                const Colour segCol = ramp (1.0f - (r.y + r.h * 0.5f) / 38.0f);   // yellow at the top, orange at the bottom
                if (lit) { g.setColour (segCol.withAlpha (0.35f)); g.strokePath (p, PathStrokeType (3.0f)); }
                g.setColour (lit ? segCol : Colour (0xff26120a));
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
        // glowing orange ring button, like the round controls in the HUD reference
        const auto r = getLocalBounds().toFloat().reduced (2.0f);
        const auto c = r.getCentre();
        g.setColour (Colours::black.withAlpha (0.6f)); g.fillEllipse (r.translated (0, 3));
        g.setColour (Colour (0xff0b0908)); g.fillEllipse (r);
        g.setColour (col::line.withAlpha (0.35f)); g.drawEllipse (r.reduced (0.5f), 1.0f);
        const auto ring = r.reduced (r.getWidth() * 0.16f);
        strokeRampRing (g, c, ring.getWidth() * 0.5f, lit ? 11.0f : 8.0f, lit ? 0.35f : 0.16f);
        strokeRampRing (g, c, ring.getWidth() * 0.5f, 3.4f, lit ? 1.0f : 0.85f);
        g.setColour (Colour (0xfffff0c8).withAlpha (lit ? 0.95f : 0.45f)); g.drawEllipse (ring, 1.0f);
        g.setColour (Colour (0xff120c08)); g.fillEllipse (ring.reduced (4.0f));
        if (lit)
        {
            g.setGradientFill (ColourGradient (col::amber.withAlpha (0.45f), c.x, c.y, col::amber.withAlpha (0.0f), c.x + ring.getWidth() * 0.5f, c.y, true));
            g.fillEllipse (ring.reduced (4.0f));
        }
        g.setGradientFill (ColourGradient (col::orange, c.x - ring.getWidth() * 0.21f, 0, col::yellow, c.x + ring.getWidth() * 0.21f, 0, false));
        g.fillRoundedRectangle (Rectangle<float> (ring.getWidth() * 0.42f, 3.2f).withCentre (c), 1.6f);
    }
    void mouseDown (const MouseEvent&) override { if (onPress) onPress (true); }
    void mouseUp (const MouseEvent&) override   { if (onPress) onPress (false); }
private:
    bool lit = false;
};

//==============================================================================
// Row of console LEDs across the preset strip that light up with the output level, orange to yellow
class LevelLights : public Component
{
public:
    void setLevel (float vuDb) { const int n = jlimit (0, count, roundToInt (jmap (vuDb, -24.0f, 3.0f, 0.0f, (float) count))); if (n != lit) { lit = n; repaint(); } }
    void paint (Graphics& g) override
    {
        const float step = (float) getWidth() / count, cy = getHeight() * 0.5f;
        for (int i = 0; i < count; ++i)
        {
            const auto r = Rectangle<float> (step * 0.62f, 4.0f).withCentre ({ step * (i + 0.5f), cy });
            const Colour c = ramp ((float) i / (count - 1));
            if (i < lit)
            {
                g.setColour (c.withAlpha (0.3f)); g.fillRoundedRectangle (r.expanded (2.5f, 2.5f), 3.0f);
                g.setColour (c.brighter (0.2f));  g.fillRoundedRectangle (r, 2.0f);
            }
            else { g.setColour (c.withAlpha (0.13f)); g.fillRoundedRectangle (r, 2.0f); }
        }
    }
private:
    static constexpr int count = 48;
    int lit = 0;
};

//==============================================================================
// Potato emblem: clean orange line art with a calm face. The eyes glow brighter while notes play.
class PotatoBadge : public Component
{
public:
    void setActivity (float a)
    {
        a = jlimit (0.0f, 1.0f, a);
        if (std::abs (a - act) > 0.01f) { act = a; repaint(); }
    }

    void paint (Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        const Point<float> c (b.getCentreX(), b.getCentreY() - 6.0f);
        const float R = jmin (b.getWidth(), b.getHeight()) * 0.37f;
        const Path spud = potatoPath (c, R);

        // backlight
        g.setGradientFill (ColourGradient (col::amber.withAlpha (0.16f + 0.14f * act), c.x, c.y, col::amber.withAlpha (0.0f), c.x + R * 1.7f, c.y, true));
        g.fillEllipse (Rectangle<float> (R * 3.4f, R * 2.7f).withCentre (c));

        // body and glowing outline
        g.setGradientFill (ColourGradient (Colour (0xff3a200c), c.x - R * 0.35f, c.y - R * 0.45f, Colour (0xff0d0704), c.x + R * 0.95f, c.y + R * 0.8f, true));
        g.fillPath (spud);
        const auto sb = spud.getBounds();
        auto rampFill = [&] (float a) { return ColourGradient (col::orange.withAlpha (a), sb.getX(), sb.getBottom(), col::yellow.withAlpha (a), sb.getRight(), sb.getY(), false); };
        g.setGradientFill (rampFill (0.16f)); g.strokePath (spud, PathStrokeType (9.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setGradientFill (rampFill (0.40f)); g.strokePath (spud, PathStrokeType (4.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setGradientFill (rampFill (1.0f));  g.strokePath (spud, PathStrokeType (1.7f, PathStrokeType::curved, PathStrokeType::rounded));

        // skin highlight and the little potato "eyes" (sprout spots)
        Path hl; hl.addCentredArc (c.x - R * 0.1f, c.y + R * 0.02f, R * 1.0f, R * 0.66f, degToRad (-14.0f), degToRad (-78.0f), degToRad (-22.0f), true);
        g.setColour (col::amberHi.withAlpha (0.35f)); g.strokePath (hl, PathStrokeType (1.2f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (col::amber.withAlpha (0.5f));
        for (auto p : { Point<float> (-0.86f, 0.34f), Point<float> (0.88f, -0.40f), Point<float> (0.66f, 0.52f), Point<float> (-0.48f, 0.62f), Point<float> (-0.10f, -0.70f) })
            g.drawEllipse (Rectangle<float> (3.4f, 2.2f).withCentre (c + p * R), 1.0f);

        // face: calm round eyes with glowing pupils, small smile
        for (int i = 0; i < 2; ++i)
        {
            const Point<float> e = c + Point<float> (i == 0 ? -0.34f : 0.30f, i == 0 ? -0.16f : -0.19f) * R;
            const auto socket = Rectangle<float> (R * 0.30f, R * 0.36f).withCentre (e);
            g.setColour (Colour (0xff080402)); g.fillEllipse (socket);
            g.setColour (col::amber.withAlpha (0.85f)); g.drawEllipse (socket, 1.3f);
            const float pr = R * (0.075f + 0.02f * act);
            const Point<float> pc = e + Point<float> (R * 0.025f, R * 0.03f);
            g.setColour (col::amber.withAlpha (0.25f + 0.4f * act)); g.fillEllipse (Rectangle<float> (pr * 4.0f, pr * 4.0f).withCentre (pc));
            g.setGradientFill (ColourGradient (Colour (0xfffff3c8), pc.x - pr * 0.3f, pc.y - pr * 0.3f, col::yellow, pc.x + pr, pc.y + pr, true));
            g.fillEllipse (Rectangle<float> (pr * 2.0f, pr * 2.0f).withCentre (pc));
            g.setColour (Colours::white.withAlpha (0.85f)); g.fillEllipse (Rectangle<float> (pr * 0.6f, pr * 0.6f).withCentre (pc - Point<float> (pr * 0.35f, pr * 0.4f)));
        }
        Path mouth;
        mouth.startNewSubPath (c.x - R * 0.22f, c.y + R * 0.30f);
        mouth.quadraticTo (c.x - R * 0.02f, c.y + R * 0.46f, c.x + R * 0.22f, c.y + R * 0.27f);
        g.setColour (col::amber.withAlpha (0.3f)); g.strokePath (mouth, PathStrokeType (4.5f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (col::yellow);                 g.strokePath (mouth, PathStrokeType (1.9f, PathStrokeType::curved, PathStrokeType::rounded));

        // HUD caption
        g.setColour (Colour (0xffb98c62)); g.setFont (mono (11.5f));
        g.drawText ("SPUD-01", Rectangle<float> (0, b.getBottom() - 14, b.getWidth(), 12), Justification::centred, false);
    }

private:
    static Path potatoPath (Point<float> c, float R)
    {
        const float radii[16] { 1.02f, 0.97f, 0.90f, 0.93f, 0.99f, 0.95f, 0.91f, 0.98f, 1.06f, 1.03f, 0.94f, 0.90f, 0.96f, 1.04f, 1.00f, 1.05f };
        const float tilt = degToRad (-10.0f), ct = std::cos (tilt), st = std::sin (tilt);
        Point<float> pts[16];
        for (int i = 0; i < 16; ++i)
        {
            const float a = degToRad (i * 22.5f);
            const float x = std::cos (a) * R * 1.22f * radii[i], y = std::sin (a) * R * 1.0f * radii[i];
            pts[i] = c + Point<float> (x * ct - y * st, x * st + y * ct);
        }
        Path p;
        auto mid = [&] (int i, int j) { return (pts[i] + pts[j]) * 0.5f; };
        p.startNewSubPath (mid (15, 0));
        for (int i = 0; i < 16; ++i) p.quadraticTo (pts[i], mid (i, (i + 1) % 16));
        p.closeSubPath();
        return p;
    }

    float act = 0.0f;
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
        g.setColour (col::line.withAlpha (0.45f)); g.drawRoundedRectangle (b.reduced (0.5f), 12, 1);
        g.setColour (col::line.withAlpha (0.2f)); g.drawVerticalLine (210, 0, b.getBottom());

        for (int i = 0; i < pk::numGenres; ++i)
        {
            const auto r = genreRect (i);
            if (i == genre) { g.setColour (Colour (0xff1e1006)); g.fillRect (r); }
            g.setFont (label (16.0f, 0.12f));
            if (i == genre) glowText (g, String (pk::genreNames[i]).toUpperCase(), r.withTrimmedLeft (22), Justification::centredLeft, col::amber, 0.2f);
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
            g.setColour (on ? col::amber : col::line.withAlpha (0.2f)); g.drawRoundedRectangle (r.reduced (0.5f), 6, 1);
            g.setColour (col::amber); g.setFont (mono (15.0f));
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
        knob (pk::id::tune,   "TUNE",       100, 225, 140);
        knob (pk::id::pamt,   "PITCH AMT",   92,  95, 292);
        knob (pk::id::pdec,   "PITCH TIME",  92, 225, 292);
        knob (pk::id::click,  "CLICK",       84,  95, 445);
        knob (pk::id::drive,  "DRIVE",       84, 225, 445);
        knob (pk::id::thr,    "THRESHOLD",   92, 975, 300);
        knob (pk::id::ratio,  "RATIO",       92, 1105, 300);
        knob (pk::id::makeup, "MAKEUP",      80, 975, 432);
        knob (pk::id::vol,    "VOLUME",      88, 1040, 550);
        knob (pk::id::atk,    "ATTACK",     100, 390, 492);   // dark purple
        knob (pk::id::dec,    "DECAY",      100, 530, 492);   // blue
        knob (pk::id::sus,    "SUSTAIN",    100, 670, 492);   // orange
        knob (pk::id::rel,    "RELEASE",    100, 810, 492);   // yellow

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

        addAndMakeVisible (levelLights);
        levelLights.setBounds (40, 625, 1000, 12);
        levelLights.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (badge);
        badge.setBounds (1058, 620, 126, 134);
        badge.setInterceptsMouseClicks (false, false);

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
        badgeLevel = active ? 1.0f : badgeLevel * 0.88f;
        badge.setActivity (badgeLevel);
        trigger.setLit (active);
        vu.setLevel (proc.meterRms.load());
        levelLights.setLevel (vu.getLevel());
        display.tick();
    }

    void paint (Graphics& g) override
    {
        g.fillAll (col::page);
        g.setGradientFill (ColourGradient (col::amber.withAlpha (0.07f), 600, 0, col::amber.withAlpha (0.0f), 600, 520, true));
        g.fillRect (0, 0, 1200, 612);

        drawTile (g, { 12, 40, 292, 566 });      // oscillator / pitch / character
        drawTile (g, { 310, 400, 580, 206 });    // amp envelope
        drawTile (g, { 896, 40, 292, 566 });     // meter / compressor / volume
        drawTile (g, { 12, 616, 1176, 140 });    // preset strip

        g.setFont (label (29.0f, 0.38f, true));
        glowTextRamp (g, "POTATO KICKER", Rectangle<float> (380, 4, 440, 32), Justification::centred, 0.35f);
        g.setFont (mono (12.0f)); g.setColour (Colour (0xffb98c62));
        g.drawText (CharPointer_UTF8 ("KICK & BASS SYNTH  \xc2\xb7  100 PRESETS"), Rectangle<float> (16, 12, 300, 16), Justification::centredLeft, false);
        g.drawText (CharPointer_UTF8 ("VST3 \xc2\xb7 AU  \xc2\xb7  WIN \xc2\xb7 MAC"), Rectangle<float> (884, 12, 300, 16), Justification::centredRight, false);
        g.setFont (label (17.0f, 0.32f));
        glowTextRamp (g, "KICK & BASS GENERATOR", Rectangle<float> (440, 577, 320, 21), Justification::centred, 0.2f);

        section (g, "OSCILLATOR", 30, 46, 256);
        section (g, "PITCH ENVELOPE", 30, 212, 256);
        section (g, "CHARACTER", 30, 362, 256);
        section (g, "COMPRESSOR", 912, 214, 260);
        section (g, "AMP ENVELOPE", 330, 408, 540);

        // strip captions
        g.setFont (label (15.0f, 0.14f));
        auto cap = [&] (const String& s, float cx, float y) { glowText (g, s, Rectangle<float> (cx - 120, y, 240, 17), Justification::centred, col::ink, 0.22f); };
        cap ("GENRE", 151, 688);
        cap ("PRESET", 356, 694);
        cap ("CLICK NAME TO BROWSE ALL 100", 593, 688);
        cap ("MIDI IN", 777, 688);
        cap ("OUTPUT", 891, 688);
        cap ("TRIGGER", 1009, 704);
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

    // Dark HUD tile with a thin orange border and bright corner brackets
    static void drawTile (Graphics& g, Rectangle<float> r)
    {
        g.setColour (Colours::black.withAlpha (0.55f)); g.fillRoundedRectangle (r.translated (0, 4), 9);
        g.setGradientFill (ColourGradient (Colour (0xff141210), 0, r.getY(), Colour (0xff0b0a09), 0, r.getBottom(), false));
        g.fillRoundedRectangle (r, 9);
        g.setGradientFill (ColourGradient (col::line.withAlpha (0.06f), 0, r.getY(), col::line.withAlpha (0.0f), 0, r.getY() + 46, false));
        g.fillRoundedRectangle (r, 9);
        g.setGradientFill (ColourGradient (col::orange.withAlpha (0.3f), r.getX(), 0, col::yellow.withAlpha (0.3f), r.getRight(), 0, false));
        g.drawRoundedRectangle (r.reduced (0.5f), 9, 1.0f);
        // light strip along the top edge
        const float lx0 = r.getX() + 26, lx1 = r.getRight() - 26;
        g.setGradientFill (ColourGradient (col::orange.withAlpha (0.0f), lx0, 0, col::orange.withAlpha (0.0f), lx1, 0, false));
        auto strip = ColourGradient (col::orange.withAlpha (0.0f), lx0, 0, col::yellow.withAlpha (0.0f), lx1, 0, false);
        strip.addColour (0.15, col::orange.withAlpha (0.9f));
        strip.addColour (0.85, col::yellow.withAlpha (0.9f));
        auto glow = strip; glow.multiplyOpacity (0.22f);
        g.setGradientFill (glow);  g.fillRoundedRectangle (lx0, r.getY() - 1.5f, lx1 - lx0, 6.0f, 3.0f);
        g.setGradientFill (strip); g.fillRect (lx0, r.getY() + 0.5f, lx1 - lx0, 1.6f);
        g.setGradientFill (ColourGradient (col::orange.withAlpha (0.95f), r.getX(), 0, col::yellow.withAlpha (0.95f), r.getRight(), 0, false));
        const float L = 16.0f, t = 2.0f, x0 = r.getX() + 3, y0 = r.getY() + 3, x1 = r.getRight() - 3, y1 = r.getBottom() - 3;
        g.fillRect (x0, y0, L, t);         g.fillRect (x0, y0, t, L);
        g.fillRect (x1 - L, y0, L, t);     g.fillRect (x1 - t, y0, t, L);
        g.fillRect (x0, y1 - t, L, t);     g.fillRect (x0, y1 - L, t, L);
        g.fillRect (x1 - L, y1 - t, L, t); g.fillRect (x1 - t, y1 - L, t, L);
    }

    static void section (Graphics& g, const String& text, float x, float y, float w)
    {
        const auto f = label (15.0f, 0.22f);
        const float tw = GlyphArrangement::getStringWidth (f, text) + 32.0f;
        const float cy = y + 7.0f, mid = x + w * 0.5f, l = mid - tw * 0.5f, r = mid + tw * 0.5f;
        g.setGradientFill (ColourGradient (col::line.withAlpha (0.0f), x, cy, col::line.withAlpha (0.45f), l, cy, false));
        g.fillRect (x, cy, l - x, 1.0f);
        g.setGradientFill (ColourGradient (col::line.withAlpha (0.45f), r, cy, col::line.withAlpha (0.0f), x + w, cy, false));
        g.fillRect (r, cy, x + w - r, 1.0f);
        drawLed (g, Rectangle<float> (5, 5).withCentre ({ l + 5, cy + 0.5f }), true);
        drawLed (g, Rectangle<float> (5, 5).withCentre ({ r - 5, cy + 0.5f }), true);
        g.setFont (f);
        glowTextRamp (g, text, Rectangle<float> (l + 10, y - 1, tw - 20, 17), Justification::centred, 0.25f);
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
    PotatoBadge badge;
    LevelLights levelLights;
    float badgeLevel = 0.0f;
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
