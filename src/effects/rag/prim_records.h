#ifndef EFFECTS_RAG_PRIM_RECORDS_H
#define EFFECTS_RAG_PRIM_RECORDS_H

#include "base.h"
#include "render/vec3.h"

#define PRIM_RECORD_COUNT 4

struct PrimStateIcon {
    int seconds_left;
    short state_id;
};

struct PrimPyramid {
    unsigned char style;
};
#define PYRAMID_STYLE_FLICKER 3

struct PrimBox {
    unsigned char active;
    short alpha;
    float full_height;
    int age;
    short angle;
    float half_x;
    float half_z;
    float height;
    Vec3 corner[4];
};

struct PrimTube {
    unsigned char active;
    short alpha;
    float radius;
    int age;
    float target;
    float style;
    Vec3 start;
    Vec3 end;
};

struct PrimOrbitSquare {
    unsigned char active;
    short alpha;
    short orbit_tilt;
    int age;
    short spin;
    float radius_speed;
    float size;
    float tilt;
    unsigned char color;
    short heading;
    float radius;
    Vec3 pos;
};

struct PrimUprightBar {
    unsigned char active;
    short alpha;
    short angle;
    int age;
    float length_speed;
    float half_width;
    unsigned char color;
    signed char shift;
    short heading;
    float length;
};

struct PrimWall {
    unsigned char active;
    short alpha;
    short phase;
    float height;
    int age;
    float style;
    Vec3 from;
    Vec3 to;
};

struct PrimSpark {
    unsigned char active;
    short alpha;
    short launch;
    float size;
    int age;
    short spin;
    unsigned char color;
    unsigned char state;
    signed char flash_shift;
    unsigned char flash;
    short heading;
    float speed;
    Vec3 pos;
    Vec3 trail_2;
    Vec3 trail_1;
    Vec3 trail_3;
};

struct PrimStripe {
    unsigned char active;
    short alpha;
    float half_width;
    int age;
    float side_shift;
    short heading;
    float length;
};

struct PrimSlideStripe {
    unsigned char active;
    short alpha;
    float half_width;
    int age;
    float side_shift;
    float start;
    float length;
    float speed;
    unsigned char color;
    short heading;
    float back;
};

struct PrimSlash {
    unsigned char active;
    short alpha;
    float radius;
    int age;
    short turn;
    float flare;
    unsigned char color;
    float width;
};

struct PrimOpenBox {
    unsigned char active;
    short alpha;
    short fade_age;
    short phase;
    float full_height;
    int age;
    short angle;
    float half_x;
    float half_z;
    float height;
    float style;
    float blue;
    float blue_add;
    float blue_falling;
    Vec3 corner[4];
};

struct PrimPrism {
    unsigned char active;
    short alpha;
    float radius;
    int age;
    short turn;
    float base_radius;
    unsigned char color;
    Vec3 base;
    Vec3 top;
};

#define HALO_LAYERS 10

struct PrimHalo {
    unsigned char active;
    int age;
    float shift;
    float layer_alpha[HALO_LAYERS];
    unsigned char texture;
    unsigned char style;
    float size;
    Vec3 corner_b;
    Vec3 corner_c;
    Vec3 corner_a;
    Vec3 corner_d;
};

struct PrimPipe {
    unsigned char active;
    short alpha;
    float radius;
    int age;
    short heading;
    float drop;
    float style;
    float length;
    Vec3 start;
    Vec3 end;
};

#define WHIP_JOINTS 21

struct PrimWhip {
    unsigned char active;
    short alpha;
    short mode;
    float thickness;
    int age;
    float bend[WHIP_JOINTS];
    signed char links[WHIP_JOINTS];
    short heading;
    float link_length;
    Vec3 root;
};

struct PrimFramedPanel {
    unsigned char active;
    short alpha;
    int age;
    unsigned char bright;
    float size;
};

struct PrimScrollGrid {
    unsigned char active;
    short alpha;
    float scroll;
    int age;
    unsigned char style;
    float cell;
};

struct PrimBladeFan {
    unsigned char active;
    short alpha;
    float height;
    int age;
    short turn;
    float inner;
    unsigned char style;
    float length;
};

struct PrimArch {
    unsigned char active;
    short alpha;
    short phase;
    float radius;
    int age;
    short turn;
    float lift;
    short wobble;
    float half_width;
    float tip_x;
    float tip_z;
};

struct PrimTiltedSquare {
    unsigned char active;
    short alpha;
    short respawns;
    float radius;
    int age;
    short turn;
    float red;
    float green;
    float blue;
    short tilt;
    float fall;
    Vec3 pos;
};

struct PrimScrollColumn {
    unsigned char active;
    short alpha;
    float steps;
    int age;
    float edge[6];
    unsigned char lengthwise;
    unsigned char style;
    signed char squash;
    float width;
    float depth;
};

struct PrimSpriteMover {
    unsigned char active;
    float speed;
    int age;
    short angle;
    float sway[2][2];
    signed char style;
    signed char style2;
    signed char mode;
    signed char follow_master;
    short bob;
    float radius;
    Vec3 centre;
    Vec3 pos;
};

struct PrimTexRecord {
    unsigned char active;
    short alpha;
    short spin;
    int age;
    short angle;
    float y;
    float ground_style;
    float pulse;
    float shape;
    float height;
    signed char texture;
    signed char look;
    signed char style;
    short bob;
    float size;
    Vec3 point[4];
};

#define PRIM_RING_COLUMNS 21

struct PrimRing {
    unsigned char active;
    short alpha;
    short mode;
    short span;
    float height;
    int age;
    short turn;
    float column[PRIM_RING_COLUMNS];
    unsigned char column_flag[PRIM_RING_COLUMNS];
    short tilt;
    float radius;
    float offset_x;
    float offset_flag;
    float offset_z;
    float ripple_start;
    float throb_base;
};

struct PrimBillboard {
    unsigned char active;
    short alpha;
    short spin;
    float range;
    int age;
    short heading;
    float wander[4];
    float speed;
    float climb;
    float height_gate;
    float style;
    float phase;
    float shift;
    unsigned char texture;
    unsigned char state;
    unsigned char color_style;
    unsigned char color_substyle;
    unsigned char circle_state;
    unsigned char rise_frames;
    float size;
    Vec3 pos;
    Vec3 local;
};

struct PrimSpiralColumn {
    unsigned char active;
    short alpha;
    short angle;
    int frame;
    float shimmer;
    unsigned char texture;
    float size;
    Vec3 pos;
};

#define SWING_ARC_POINTS 21

struct PrimSwingArc {
    unsigned char active;
    short alpha;
    short clockwise;
    short style;
    float width;
    int frame;
    float points[SWING_ARC_POINTS];
    unsigned char marks[SWING_ARC_POINTS];
    short heading;
    float size;
    Vec3 pos;
    float height_scale;
    float count;
};

#define CROWN_RING_POINTS 21

struct PrimCrownRing {
    unsigned char active;
    short alpha;
    short arc;
    float base_height;
    int frame;
    short arc_start;
    float heights[CROWN_RING_POINTS];
    short tilt;
    float radius;
    Vec3 pos;
    Vec3 offset;
};

struct PrimStretchRay {
    unsigned char active;
    short alpha;
    short angle;
    int frame;
    short stretch;
    unsigned char texture;
    float size;
    Vec3 pos;
    float height;
};

struct PrimAnimatedPanel {
    signed char active;
    short alpha;
    int frame;
    short angle;
    float size;
};

#define DRIFT_ORB_AXES 3

struct PrimDriftOrb {
    unsigned char active;
    short alpha;
    short sway;
    float grow;
    int frame;
    short angle;
    float drift[DRIFT_ORB_AXES][2];
    float spin;
    float lift;
    float shimmer;
    float wide;
    float height;
    signed char style;
    short blade_alpha;
    float size;
    Vec3 offset;
    Vec3 foot_far;
    Vec3 foot_near;
};

#define WAVE_RING_POINTS 21

struct PrimWaveRing {
    unsigned char active;
    unsigned short alpha;
    short style;
    short arc;
    float base_height;
    int frame;
    short arc_start;
    float heights[WAVE_RING_POINTS];
    short tilt;
    float radius;
    Vec3 walked;
    float strip_style;
};

struct PrimSpinSquare {
    unsigned char active;
    unsigned short alpha;
    int frame;
    short angle;
    float y;
    float texture;
    float still;
    short pulse;
    float size;
    Vec3 corners[4];
};

struct PrimWander {
    float angle;
    float target;
};

struct PrimSwingTube {
    unsigned char active;
    short alpha;
    float radius;
    int frame;
    float arc[21];
    short yaw;
    float reach;
    Vec3 centre;
    float height;
    float points;
};

struct PrimSquareStack {
    unsigned char active;
    short alpha;
    float radius;
    int age;
    short turn;
    float top;
    float y_step;
    signed char layers;
    signed char style;
    signed char fade;
    float shrink;
};

struct PrimEnergyArc {
    unsigned char active;
    short alpha;
    short arc;
    float drop;
    int age;
    short turn;
    float width;
    float tilt;
    short start;
    float radius;
    Vec3 centre;
};

struct PrimLaunchSquare {
    unsigned char active;
    int frame;
    float aid;
    float alpha;
    float size;
    float angle;
    float dist;
    float y;
};

struct PrimSparkTrio {
    unsigned char active;
    float speed;
    int frame;
    struct PrimWander turn[6];
    float alpha[3];
    float tint;
    float size;
    Vec3 spot[4];
};

struct PrimGroundTarget {
    unsigned char active;
    float scale;
    short turn;
};

struct PrimSpinBox {
    unsigned char active;
    short alpha;
    short turn;
    float y;
    unsigned char grey;
    short pulse;
    float size;
    Vec3 corner[4];
};

struct PrimFlatSquare {
    unsigned char active;
    short alpha;
    int frame;
    short heading;
    float speed;
    signed char state;
    unsigned char turn_up;
    unsigned char style;
    unsigned char fades_early;
    short yaw;
    float size;
    Vec3 pos;
};

struct PrimEffectSpawner {
    unsigned char active;
    int frame;
    short heading;
    float speed;
    unsigned char mode;
    Vec3 at;
};

struct PrimArcWall {
    unsigned char active;
    short alpha;
    short look;
    short arc;
    float full_height;
    int age;
    short start;
    float height[21];
    unsigned char risen[21];
    short lean;
    float radius;
};

struct PrimWideRing {
    unsigned char active;
    short alpha;
    float radius;
    int frame;
    short angle;
    float base_radius;
    unsigned char style;
    Vec3 outer_centre;
    Vec3 inner_centre;
};

struct PrimWanderGlows {
    unsigned char active;
    int frame;
    struct PrimWander wander[8];
    unsigned char mode;
    unsigned char style;
    signed char flash[4];
    signed char alpha[4];
    float size;
    Vec3 glow[4];
};

struct PrimNestedSquares {
    unsigned char active;
    int frame;
    float shift_x;
    float alpha[10];
    unsigned char texture;
    unsigned char style;
    float size;
};

struct PrimStandingSquare {
    unsigned char active;
    short alpha;
    short phase;
    float grow;
    int frame;
    short yaw;
    float width;
    float shade;
    float drop;
    float style;
    Vec3 top_back;
    Vec3 top;
};

struct PrimShapeRow {
    unsigned char active;
    short alpha;
    short angle;
    int frame;
    float wobble;
    float shape;
    signed char texture;
    signed char look;
    signed char pace;
    short heading;
    float size;
    Vec3 target;
    float start_y;
};

struct PrimRisingMotes {
    unsigned char active;
    short yaw;
    float rise;
    float drift[8][2];
    signed char tint[3];
    short size10;
    float size;
    Vec3 motes[4];
};

struct PrimOrbitLauncher {
    unsigned char active;
    int frame;
    float owner;
    float alpha;
    unsigned char style;
    float size;
    float radius;
    Vec3 pos;
};

struct PrimSpriteGhosts {
    signed char active;
    int frame;
    short spin_a;
    float drift[6];
    float target[6];
    unsigned char style;
    short spin_b;
    float speed;
    Vec3 offsets[4];
};

struct PrimSpriteHalo {
    signed char active;
    int frame;
    unsigned char additive;
};

struct PrimWedgePair {
    unsigned char active;
    short alpha;
    float side;
    int frame;
    float reach;
    unsigned char style;
    short angle;
};

struct PrimWispCourse {
    float base_yaw;
    float yaw_target;
    float yaw;
    float pitch_target;
    float pitch;
};

struct PrimTwinWisps {
    unsigned char active;
    float speed;
    int frame;
    struct PrimWispCourse course[2];
    float alpha[2];
    unsigned char style;
    short heading;
    float size;
    Vec3 pos1;
    Vec3 pos0;
};

struct PrimFanQuad {
    unsigned char active;
    unsigned short alpha;
    short angle;
    int frame;
    short bob;
    float spin;
    float shape;
    unsigned char texture;
    signed char jitter_style;
    signed char fan_style;
    unsigned char quick;
    float size;
    Vec3 pos;
    float base_y;
};

struct PrimDropSquare {
    unsigned char active;
    unsigned short alpha;
    short angle;
    float fall;
    int frame;
    float radius;
    unsigned char style;
    float size;
    Vec3 pos;
};

struct PrimWhirlDisc {
    unsigned char active;
    unsigned short alpha;
    int frame;
    short pitch;
    float lift;
    float speed;
    unsigned char phase;
    signed char spins;
    unsigned char tint;
    short yaw;
    float size;
    Vec3 pos;
};

struct PrimDashLine {
    unsigned char active;
    unsigned short alpha;
    float length;
    int frame;
    short pitch;
    float y;
    float texture;
    float trail;
    short yaw;
    float reach;
};

struct PrimRibbon {
    unsigned char active;
    short alpha;
    int frame;
    float y;
    float texture;
    float scroll_speed;
    float tone;
    float scroll;
    short angle;
    float half_width;
};

struct PrimFlyingSprite {
    unsigned char active;
    short heading;
    float speed;
    unsigned char tint;
    Vec3 offset;
};

struct PrimFacingSquare {
    unsigned char active;
    short alpha;
    int age;
    float wave_x;
    float wave_z;
    float wave_y;
    float orbit;
    float lift;
    float pulse;
    float fading;
    float size;
    Vec3 centre;
};

#define SWELL_RING_SEGMENTS 20

struct PrimSwellRing {
    unsigned char active;
    short alpha;
    short arc;
    float radius;
    int frame;
    short spin;
    float swell[SWELL_RING_SEGMENTS + 1];
    short tilt;
    float rim;
};

struct PrimSway {
    float angle;
    float target;
};

struct PrimLightGroup {
    unsigned char active;
    struct PrimSway sway[2];
    float angle[4];
    unsigned char style;
    float size;
    Vec3 point[4];
};

struct PrimAuraGroup {
    unsigned char active;
    float speed;
    struct PrimSway sway[8];
    signed char style;
    signed char drift_count[4];
    unsigned char tint;
    unsigned char swap_xz;
    unsigned char narrow;
    float size;
    Vec3 point[4];
};

struct PrimGlowOrb {
    unsigned char active;
    unsigned short brightness;
    int frames;
    struct PrimSway sway[3];
    float spin;
    float height;
    float shimmer;
    float close_orbit;
    signed char style;
    float size;
    Vec3 point;
};

#define RING_WALL_STRIPS 21

struct PrimRingWall {
    unsigned char active;
    unsigned short alpha;
    unsigned short span;
    float full_height;
    int age;
    short start;
    float height[RING_WALL_STRIPS];
    signed char offset_x;
    signed char offset_z;
    short tilt;
    float radius;
    Vec3 foot;
    Vec3 top;
    Vec3 last_foot;
    Vec3 last_top;
};

struct PrimSparkGroup {
    unsigned char active;
    int frames;
    struct {
        float age;
        float alpha;
        float texture;
        float angle;
    } spark[4];
    float lift;
    unsigned char tint;
    float size;
    Vec3 point[4];
};

struct PrimSpike {
    unsigned char active;
    short alpha;
    short swell;
    float swell_speed;
    int age;
    short turn;
    float red;
    float green;
    float blue;
    float depth;
    short tilt;
    float length;
};

struct PrimTurnedSquare {
    unsigned char active;
    short alpha;
    int age;
    short turn;
    float speed;
    unsigned char style;
    float radius;
    Vec3 pos;
};

struct PrimFanWedge {
    unsigned char active;
    short alpha;
    int age;
    short turn;
    float height;
    float style;
    short heading;
    float length;
};

struct PrimLightBox {
    unsigned char active;
    short alpha;
    short sink_frame;
    float height_max;
    int frame;
    float half_x;
    float half_z;
    float height;
    float run_len;
    float style;
    float run;
    Vec3 corner[4];
};

struct PrimRainbow {
    unsigned char active;
    short alpha;
    short view_yaw;
    short arc;
    int age;
    short start;
    short yaw;
    float radius;
};

struct PrimDriftSquare {
    unsigned char active;
    short alpha;
    float fall;
    int frame;
    float drift_x;
    float drift_x_to;
    float drift_z;
    float drift_z_to;
    unsigned char tint;
    short heading;
    float size;
    float base_y;
    Vec3 centre;
};

struct PrimUprightSquare {
    unsigned char active;
    short alpha;
    int age;
    short yaw;
    float size;
    Vec3 centre;
};

#define ARC_TUBE_POINTS 21

struct PrimArcTube {
    unsigned char active;
    short alpha;
    short style;
    short style_jagged;
    float radius;
    int frame;
    float arc[ARC_TUBE_POINTS];
    short yaw;
    float reach;
    Vec3 centre;
    float height;
    float points;
};

union PrimRecords {
    struct PrimLightGroup light_groups[PRIM_RECORD_COUNT];
    struct PrimAuraGroup aura_groups[PRIM_RECORD_COUNT];
    struct PrimGlowOrb glow_orb;
    struct PrimRingWall ring_walls[PRIM_RECORD_COUNT];
    struct PrimSparkGroup spark_groups[PRIM_RECORD_COUNT];
    struct PrimSpike spikes[PRIM_RECORD_COUNT];
    struct PrimTurnedSquare turned_squares[PRIM_RECORD_COUNT];
    struct PrimFanWedge fan_wedges[PRIM_RECORD_COUNT];
    struct PrimLightBox light_boxes[PRIM_RECORD_COUNT];
    struct PrimRainbow rainbows[PRIM_RECORD_COUNT];
    struct PrimDriftSquare drift_squares[PRIM_RECORD_COUNT];
    struct PrimUprightSquare upright_squares[PRIM_RECORD_COUNT];
    struct PrimArcTube arc_tubes[PRIM_RECORD_COUNT];
    struct PrimRisingMotes rising_motes[PRIM_RECORD_COUNT];
    struct PrimOrbitLauncher orbit_launchers[PRIM_RECORD_COUNT];
    struct PrimSpriteGhosts sprite_ghosts[PRIM_RECORD_COUNT];
    struct PrimSpriteHalo sprite_halos[PRIM_RECORD_COUNT];
    struct PrimWedgePair wedge_pairs[PRIM_RECORD_COUNT];
    struct PrimTwinWisps twin_wisps[PRIM_RECORD_COUNT];
    struct PrimFanQuad fan_quads[PRIM_RECORD_COUNT];
    struct PrimDropSquare drop_squares[PRIM_RECORD_COUNT];
    struct PrimWhirlDisc whirl_discs[PRIM_RECORD_COUNT];
    struct PrimDashLine dash_lines[PRIM_RECORD_COUNT];
    struct PrimRibbon ribbons[PRIM_RECORD_COUNT];
    struct PrimFlyingSprite flying_sprites[PRIM_RECORD_COUNT];
    struct PrimFacingSquare facing_square;
    struct PrimSwellRing swell_rings[PRIM_RECORD_COUNT];
    struct PrimSwingTube swing_tubes[PRIM_RECORD_COUNT];
    struct PrimSquareStack square_stacks[PRIM_RECORD_COUNT];
    struct PrimEnergyArc energy_arcs[PRIM_RECORD_COUNT];
    struct PrimLaunchSquare launch_squares[PRIM_RECORD_COUNT];
    struct PrimSparkTrio spark_trios[PRIM_RECORD_COUNT];
    struct PrimGroundTarget ground_targets[PRIM_RECORD_COUNT];
    struct PrimSpinBox spin_boxes[PRIM_RECORD_COUNT];
    struct PrimFlatSquare flat_squares[PRIM_RECORD_COUNT];
    struct PrimEffectSpawner effect_spawners[PRIM_RECORD_COUNT];
    struct PrimArcWall arc_walls[PRIM_RECORD_COUNT];
    struct PrimWideRing wide_rings[PRIM_RECORD_COUNT];
    struct PrimWanderGlows wander_glows[PRIM_RECORD_COUNT];
    struct PrimNestedSquares nested_squares[PRIM_RECORD_COUNT];
    struct PrimStandingSquare standing_squares[PRIM_RECORD_COUNT];
    struct PrimShapeRow shape_rows[PRIM_RECORD_COUNT];
    struct PrimSpiralColumn spiral_columns[PRIM_RECORD_COUNT];
    struct PrimSwingArc swing_arcs[PRIM_RECORD_COUNT];
    struct PrimCrownRing crown_rings[PRIM_RECORD_COUNT];
    struct PrimStretchRay stretch_rays[PRIM_RECORD_COUNT];
    struct PrimAnimatedPanel animated_panels[PRIM_RECORD_COUNT];
    struct PrimDriftOrb drift_orbs[PRIM_RECORD_COUNT];
    struct PrimWaveRing wave_rings[PRIM_RECORD_COUNT];
    struct PrimSpinSquare spin_squares[PRIM_RECORD_COUNT];
    struct PrimRing rings[PRIM_RECORD_COUNT];
    struct PrimBillboard billboards[PRIM_RECORD_COUNT];
    struct PrimStateIcon state_icon;
    struct PrimPyramid pyramid;
    struct PrimBox boxes[PRIM_RECORD_COUNT];
    struct PrimTube tube;
    struct PrimOrbitSquare orbit_squares[PRIM_RECORD_COUNT];
    struct PrimUprightBar upright_bars[PRIM_RECORD_COUNT];
    struct PrimWall walls[PRIM_RECORD_COUNT];
    struct PrimSpark sparks[PRIM_RECORD_COUNT];
    struct PrimStripe stripes[PRIM_RECORD_COUNT];
    struct PrimSlideStripe slide_stripes[PRIM_RECORD_COUNT];
    struct PrimSlash slashes[PRIM_RECORD_COUNT];
    struct PrimOpenBox open_boxes[PRIM_RECORD_COUNT];
    struct PrimPrism prisms[PRIM_RECORD_COUNT];
    struct PrimHalo halos[PRIM_RECORD_COUNT];
    struct PrimPipe pipes[PRIM_RECORD_COUNT];
    struct PrimWhip whips[PRIM_RECORD_COUNT];
    struct PrimFramedPanel framed_panels[PRIM_RECORD_COUNT];
    struct PrimScrollGrid scroll_grids[PRIM_RECORD_COUNT];
    struct PrimBladeFan blade_fans[PRIM_RECORD_COUNT];
    struct PrimArch arches[PRIM_RECORD_COUNT];
    struct PrimTiltedSquare tilted_squares[PRIM_RECORD_COUNT];
    struct PrimScrollColumn scroll_columns[PRIM_RECORD_COUNT];
    struct PrimSpriteMover sprite_mover;
    struct PrimTexRecord tex_records[PRIM_RECORD_COUNT];
};

#endif
