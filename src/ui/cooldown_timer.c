#include "ui/cooldown.h"
#include <math.h>

void cooldown_timer_init(struct CooldownTimer *t, int slot, int x, int y,
    unsigned total_ms, unsigned start, unsigned elapsed, int mode)
{
    *t = (struct CooldownTimer){0};
    t->total_ms = total_ms;
    t->start = start;
    t->elapsed = elapsed;
    t->mode = mode;
    t->vertex_count = PIE_VERTICES;
    t->x = x;
    t->y = y;
    t->slot = slot;
    t->color = mode == 0 ? 0xaa000000 : 0xaaffffff;
}

static void timer_add_edge(struct CooldownTimer *t, unsigned segment, float base, int reach, int add)
{
    unsigned mask = 1u << segment;
    if (!(t->edge_mask & mask)) {
        t->edges[segment] = (struct PieEdge){base, base, reach, add};
        t->edge_mask |= mask;
    }
}

static float timer_update(struct CooldownTimer *t, unsigned clock)
{
    unsigned now = t->elapsed - t->start + clock;
    unsigned total = t->total_ms;
    struct PieEdge *e;

    if (total - now <= 60000 && t->last_minute == 0 && t->mode == 1) {
        now = now - total + 60000;
        t->last_minute = 1;
        t->total_ms = 60000;
        t->elapsed = now;
        t->start = clock;
        now = t->elapsed;
        total = t->total_ms;
        t->angle = 0.0f;
        t->color = 0xaaff6633;
    }
    if (total) {
        float angle = 0.0f;
        int full, reversed;

        if (now) {
            angle = (float)now / (float)total;
            angle *= 360.0f;
        }
        t->angle = angle;
        if (angle >= 0.0f && 45.0f > angle) {
            t->segment = 0, full = 0x18, reversed = 3;
        } else if (angle >= 45.0f && 90.0f > angle) {
            t->segment = 1, full = 0x15, reversed = 6;
        } else if (angle >= 90.0f && 135.0f > angle) {
            t->segment = 2, full = 0x12, reversed = 9;
        } else if (angle >= 135.0f && 180.0f > angle) {
            t->segment = 3, full = 0xf, reversed = 0xc;
        } else if (angle >= 180.0f && 225.0f > angle) {
            t->segment = 4, full = 0xc, reversed = 0xf;
        } else if (angle >= 225.0f && 270.0f > angle) {
            t->segment = 5, full = 9, reversed = 0x12;
        } else if (angle >= 270.0f && 315.0f > angle) {
            t->segment = 6, full = 6, reversed = 0x15;
        } else {
            t->segment = 7, full = 3, reversed = 0x18;
        }
        t->vertex_count = t->mode ? reversed : full;
        t->angle = 90.0f - (t->angle - (float)t->segment * 45.0f);
        total = t->total_ms;
    }
    e = &t->edges[t->segment];
    if (total > now) {
        float reach = (float)e->reach;
        float tangent = (float)tan((double)(t->angle * 0.017453292f));
        float edge = e->add ? reach / tangent + e->base : e->base - reach / tangent;

        e->edge = edge;
        return edge;
    }
    return e->base;
}

static int timer_layout_icon(struct CooldownTimer *t, unsigned clock)
{
    struct PieVertex *v = t->verts;
    int i;

    if (t->total_ms == 0)
        return 0;
    switch (t->segment) {
    case 0:
        v[2].x = timer_update(t, clock);
        if (t->segment != 0) {
            v[5] = v[4];
            v[2] = v[4];
        }
        break;
    case 1:
        v[5].y = timer_update(t, clock);
        if (t->segment != 1) {
            v[8] = v[7];
            v[5] = v[7];
        }
        break;
    case 2:
        v[8].y = timer_update(t, clock);
        if (t->segment != 2) {
            v[11] = v[10];
            v[8] = v[10];
        }
        break;
    case 3:
        v[11].x = timer_update(t, clock);
        if (t->segment != 3) {
            v[14] = v[13];
            v[11] = v[13];
        }
        break;
    case 4:
        v[14].x = timer_update(t, clock);
        if (t->segment != 4) {
            v[17] = v[16];
            v[14] = v[16];
        }
        break;
    case 5:
        v[17].y = timer_update(t, clock);
        if (t->segment != 5) {
            v[20] = v[19];
            v[17] = v[19];
        }
        break;
    case 6:
        v[20].y = timer_update(t, clock);
        if (t->segment != 6) {
            v[23] = v[22];
            v[20] = v[22];
        }
        break;
    case 7:
        v[23].x = timer_update(t, clock);
        break;
    }
    for (i = 0; i < PIE_VERTICES; i++) {
        v[i].x += (float)t->x;
        v[i].y += (float)t->y;
    }
    return 1;
}

int cooldown_timer_layout(struct CooldownTimer *t, unsigned clock, int x, int y,
    int w, int h, int limit, int dx, int dy)
{
    struct PieVertex *v = t->verts;
    float fx, fy, top, bottom, left, right;
    int i;

    if (t->x <= 0 || t->y <= 0 || !(t->color & 0xff000000))
        return 0;
    fx = (float)x;
    top = (float)(y - h);
    bottom = (float)(y + h);
    v[1].x = fx;
    v[23].x = fx;
    v[1].y = top;
    v[23].y = top;
    v[11].x = fx;
    v[13].x = fx;
    v[11].y = bottom;
    v[13].y = bottom;
    fy = 0.0f;
    for (i = 0; i < PIE_VERTICES; i++) {
        fy = (float)y;
        if (i % 3 == 0) {
            v[i].x = fx;
            v[i].y = fy;
        }
        v[i].z = 0.000001f;
        v[i].rhw = 0.999999f;
        v[i].u = 0.0f;
        v[i].v = 0.0f;
        v[i].color = t->color;
    }
    v[5].y = fy;
    v[7].y = fy;
    v[2].y = top;
    v[4].y = top;
    v[8].y = bottom;
    v[10].y = bottom;
    v[14].y = bottom;
    v[16].y = bottom;
    v[17].y = fy;
    v[19].y = fy;
    v[20].y = top;
    v[22].y = top;
    right = (float)(x + w);
    left = (float)(x - w);
    if (t->mode == 0) {
        v[2].x = v[4].x = v[5].x = v[7].x = v[8].x = v[10].x = left;
        v[14].x = v[16].x = v[17].x = v[19].x = v[20].x = v[22].x = right;
        timer_add_edge(t, 0, v[23].x, w, 1);
        timer_add_edge(t, 1, v[20].y, h, 1);
        timer_add_edge(t, 2, v[17].y, h, 1);
        timer_add_edge(t, 3, v[14].x, w, 0);
        timer_add_edge(t, 4, v[11].x, w, 0);
        timer_add_edge(t, 5, v[8].y, h, 0);
        timer_add_edge(t, 6, v[5].y, h, 0);
        timer_add_edge(t, 7, v[2].x, w, 1);
        if (t->total_ms == 0)
            return 0;
        switch (t->segment) {
        case 0: v[23].x = timer_update(t, clock); break;
        case 1: v[20].y = timer_update(t, clock); break;
        case 2: v[17].y = timer_update(t, clock); break;
        case 3: v[14].x = timer_update(t, clock); break;
        case 4: v[11].x = timer_update(t, clock); break;
        case 5: v[8].y = timer_update(t, clock); break;
        case 6: v[5].y = timer_update(t, clock); break;
        case 7: v[2].x = timer_update(t, clock); break;
        }
        if ((limit ? limit : 6) <= t->y)
            return 0;
        for (i = 0; i < PIE_VERTICES; i++) {
            v[i].x += (float)(t->x + 0xc + dx);
            v[i].y += (float)(t->y + 0xc + dy);
        }
        return 1;
    }
    v[2].x = v[4].x = v[5].x = v[7].x = v[8].x = v[10].x = right;
    v[14].x = v[16].x = v[17].x = v[19].x = v[20].x = v[22].x = left;
    timer_add_edge(t, 0, v[1].x, w, 1);
    timer_add_edge(t, 1, v[4].y, h, 1);
    timer_add_edge(t, 2, v[7].y, h, 1);
    timer_add_edge(t, 3, v[10].x, w, 0);
    timer_add_edge(t, 4, v[13].x, w, 0);
    timer_add_edge(t, 5, v[16].y, h, 0);
    timer_add_edge(t, 6, v[19].y, h, 0);
    timer_add_edge(t, 7, v[22].x, w, 1);
    return timer_layout_icon(t, clock);
}
