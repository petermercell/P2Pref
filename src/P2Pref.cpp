// P2Pref.cpp
// Converts position pass (P) to reference frame position (Pref)

static const char* const HELP = 
    "Converts position pass (P) to reference frame position (Pref).\n"
    "Step 1: Applies inverted transform from axis at current frame.\n"
    "Step 2: Applies inverted rotation from inverted axis at reference frame.\n"
    "Step 3: Applies inverted translation from inverted axis at reference frame.\n";

#include "DDImage/PixelIop.h"
#include "DDImage/CameraOp.h"
#include "DDImage/AxisOp.h"
#include "DDImage/Row.h"
#include "DDImage/Knobs.h"
#include "DDImage/Matrix4.h"

using namespace DD::Image;

class P2Pref : public PixelIop
{
    Matrix4 _axis_transform;
    Matrix4 _axis_rotation_inv;
    Matrix4 _axis_translation_inv;
    int _reference_frame;
    CameraOp* _cam_op;
    AxisOp* _axis_op;

public:
    P2Pref(Node* node) : PixelIop(node),
        _reference_frame(1),
        _cam_op(nullptr),
        _axis_op(nullptr)
    {
        _axis_transform.makeIdentity();
        _axis_rotation_inv.makeIdentity();
        _axis_translation_inv.makeIdentity();
    }

    bool pass_transform() const { return true; }
    
    virtual int minimum_inputs() const { return 2; } // img + axis
    virtual int maximum_inputs() const { return 2; }
    
    virtual void knobs(Knob_Callback f);
    
    static const Iop::Description d;
    const char* Class() const { return d.name; }
    const char* node_help() const { return HELP; }

    void _validate(bool);
    void _request(int x, int y, int r, int t, ChannelMask channels, int count);
    
    void in_channels(int input, ChannelSet& mask) const {
        if (input == 0) {
            mask += Mask_RGBA;
        }
    }

    void pixel_engine(const Row& in, int y, int x, int r, ChannelMask channels, Row& out);

    bool test_input(int n, Op* op) const {
        if (n >= 1) {
            return (dynamic_cast<CameraOp*>(op) != 0) || (dynamic_cast<AxisOp*>(op) != 0);
        }
        return Iop::test_input(n, op);
    }

    Op* default_input(int input) const {
        if (input == 1) {
            return CameraOp::default_camera();
        }
        return Iop::default_input(input);
    }

    const char* input_label(int input, char* buffer) const {
        switch (input) {
            case 0: return "P";
            case 1: return "axis";
        }
        return nullptr;
    }
};

void P2Pref::_validate(bool for_real)
{
    copy_info();

    // Get axis from input 1 and validate it
    Op* inputOp = Op::input(1);
    _cam_op = dynamic_cast<CameraOp*>(inputOp);
    _axis_op = dynamic_cast<AxisOp*>(inputOp);
    
    if (_cam_op) {
        _cam_op->validate(for_real);
        // Get inverted transform at current frame
        _axis_transform = _cam_op->matrix().inverse();
        
    } else if (_axis_op) {
        _axis_op->validate(for_real);
        // Get inverted transform at current frame
        _axis_transform = _axis_op->matrix().inverse();
        
    } else {
        // No camera/axis - use identity matrix
        _axis_transform.makeIdentity();
    }

    // Output RGBA channels
    set_out_channels(Mask_RGBA);
    info_.turn_on(Mask_RGBA);
    info_.black_outside(true);
}

void P2Pref::_request(int x, int y, int r, int t, ChannelMask channels, int count)
{
    // Request RGBA from input
    ChannelSet request_chans = Mask_RGBA;
    input0().request(x, y, r, t, request_chans, count);
}

void P2Pref::pixel_engine(const Row& in, int y, int x, int r, ChannelMask channels, Row& out)
{
    if (aborted())
        return;

    // Get rotation matrix from inverted axis input (input 2) and invert it
    Matrix4 axis_rotation_inv;
    axis_rotation_inv.makeIdentity();
    
    // Get translation matrix from inverted axis input (input 2) and invert it
    Matrix4 axis_translation_inv;
    axis_translation_inv.makeIdentity();
    
    if (_cam_op || _axis_op) {
        // Get the knobs from axis
        Knob* translate_knob = (_cam_op ? _cam_op : _axis_op)->knob("translate");
        Knob* rotate_knob = (_cam_op ? _cam_op : _axis_op)->knob("rotate");
        
        if (translate_knob && rotate_knob) {
            // Get translate and rotate at reference frame and invert them
            Vector3 ref_translate(
                -1.0 * translate_knob->get_value_at(_reference_frame, 0),
                -1.0 * translate_knob->get_value_at(_reference_frame, 1),
                -1.0 * translate_knob->get_value_at(_reference_frame, 2)
            );
            
            Vector3 ref_rotate(
                -1.0 * rotate_knob->get_value_at(_reference_frame, 0),
                -1.0 * rotate_knob->get_value_at(_reference_frame, 1),
                -1.0 * rotate_knob->get_value_at(_reference_frame, 2)
            );
            
            // Build rotation matrix from inverted axis (YXZ order - matches Nuke's ZXY application)
            Matrix4 rotation_matrix;
            rotation_matrix.makeIdentity();
            rotation_matrix.rotateY(radians(ref_rotate.y));
            rotation_matrix.rotateX(radians(ref_rotate.x));
            rotation_matrix.rotateZ(radians(ref_rotate.z));
            // Then invert it (Step 2)
            axis_rotation_inv = rotation_matrix.inverse();
            
            // Build translation matrix from inverted axis
            Matrix4 translation_matrix;
            translation_matrix.makeIdentity();
            translation_matrix.translate(ref_translate.x, ref_translate.y, ref_translate.z);
            // Then invert it (Step 3)
            axis_translation_inv = translation_matrix.inverse();
        }
    }

    // Input channels
    const float* R = in[Chan_Red];
    const float* G = in[Chan_Green];
    const float* B = in[Chan_Blue];
    const float* A = in[Chan_Alpha];

    // Output channels
    float* outR = out.writable(Chan_Red);
    float* outG = out.writable(Chan_Green);
    float* outB = out.writable(Chan_Blue);
    float* outA = out.writable(Chan_Alpha);

    // Process each pixel
    for (int X = x; X < r; X++) {
        // Read input
        Vector4 v(R[X], G[X], B[X], A[X]);
        
        // Step 1: Apply inverted transform from current frame axis
        v = _axis_transform.transform(v);
        
        // Step 2: Apply inverted rotation from inverted axis
        v = axis_rotation_inv.transform(v);
        
        // Step 3: Apply inverted translation from inverted axis
        v = axis_translation_inv.transform(v);
        
        // Write output
        outR[X] = v.x;
        outG[X] = v.y;
        outB[X] = v.z;
        outA[X] = v.w;
    }
}

void P2Pref::knobs(Knob_Callback f)
{
    Int_knob(f, &_reference_frame, "reference_frame", "reference frame");
    Tooltip(f, "Reference frame for inverting axis rotation and translation.");
    
    Divider(f, "");
    Text_knob(f, "P2Pref by Peter Mercell 2025\nInspired by Ivan Busquets and Comp Lair Pedro Andrade");
}

static Iop* build(Node* node) { 
    return new P2Pref(node); 
}

const Iop::Description P2Pref::d("P2Pref", "Color/P2Pref", build);
