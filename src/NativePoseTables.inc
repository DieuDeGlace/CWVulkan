// The targets of every pose state (docs/specs/creature-pose-tables.md), included by
// NativePose.cpp inside its Poser class's namespace. Each state's entry runs once per call,
// after gliding and before the common steps. Rows of a state are tested top to bottom and
// only the first that holds applies; `until(x)` is the spec's "x > t". A state sets targets
// (RT, LT, RS, LS, BT, WA, BS, HS, HA, TA, ST, WS, KS, DR), may move the rest places (Rr, Lr,
// Rm, Lm, Fr, Fl, Tp, Hp) and may change sprites, `splay`, `turnOut`, `heldScale` and `rate`.

// ---------------------------------------------------------------------------------------
// Shared blocks.

// Both hands on one weapon.
void Poser::grip() {
    LS = RS;
    LT = RT;
    Lr = Rr;
    Lr.x += -0.2f, Lr.y += -0.2f, Lr.z += -1;
}

// G: the free hand rests on the one weapon held.
void Poser::oneWeaponGrip() {
    if (!oneWeapon())
        return;
    Rr.y += 2, Rr.z += 3;
    Lr = Rr;
    Lr.x += -3, Lr.z += -3;
    RT.x = 0, RT.y = 20;
    Lm.y += 10, Lm.z += -4;
    LT = RT;
}

// S (and S' with `leftShieldTurn`): standing up again after a blow.
void Poser::standUp(bool leftShieldTurn) {
    BT.z = 20;
    RT = V(-120, -90, 30);
    LT = leftShieldTurn && leftKind() == 0xd ? V(0, 0, 90) : V(-80, 40, 60);
    LS.y += -4, LS.z += 2;
    oneWeaponGrip();
}

// The blow is over: back to state 0.
void Poser::ended() {
    ST = V(0, 0, 0);
    endState();
}

// Hands circling in front, optionally with the body bobbing at frequency b.
void Poser::circle(bool withBob, float b) {
    BT.z = 0, WA.z = 0;
    RT = V(60, -20, 20);
    const float u = t * 0.01f;
    RS = V(0, nativecrt::sinf(u) + 2.0f, nativecrt::cosf(u) + 4.0f);
    LT = V(60, 20, -20);
    const float v = (float)((double)(t * 0.01f) + HALF_PI);
    LS = V(0, nativecrt::sinf(v) + 2.0f, nativecrt::cosf(v) + 4.0f);
    if (withBob)
        BS.z += nativecrt::sinf(t * b) * 0.5f;
}

// Bob: chewing while eating or drinking.
void Poser::bobWhileEating() {
    WS.z = (float)((double)nativecrt::cosf(t * 0.003f) * 0.2 + 0.800000011920929);
    KS = -30;
    Fr.y += 4, Fl.y += 4;
}

// H: hands together in front, circling.
void Poser::handsCircling() {
    RT = V(0, -45, -60);
    BT.z = -30;
    LT = V(-45, -45, 0);
    RS.y += 4, LS.y += 4;
    RS.x += nativecrt::sinf(t * 0.04f) * 0.5f;
    RS.z += nativecrt::cosf(t * 0.05f) * 0.5f;
    LS.x += nativecrt::sinf(t * 0.04f) * 0.5f;
    LS.z += nativecrt::cosf(t * 0.05f) * 0.5f;
}

// Gather: head, hands, feet and tail drawn in toward the chest by k (state 0x2a).
void Poser::gather(float k) {
    auto toward = [k](Vec3 from, Vec3 to) {
        return Vec3{k * (to.x - from.x) + from.x, k * (to.y - from.y) + from.y, k * (to.z - from.z) + from.z};
    };
    Hp = toward(Hp, V(Cp.x - 0.0f, Cp.y - 0.0f, Cp.z - 2.0f));
    Lr = toward(Lr, Cp);
    Rr = toward(Rr, Cp);
    Fl = toward(Fl, Cp);
    Fr = toward(Fr, Cp);
    Tp = toward(Tp, add(V(0, 4, 4), Cp));
}

// Every third swing spins (states 0xf, 0x10, 0x1f, 0x20, 0x31, 0x32).
static int swingTurn(const uint8_t *c) { return (get<int32_t>(c, SWING_COUNT) / 2) % 3; }

// ---------------------------------------------------------------------------------------
// States 1 - 9.

// 1: a charged blow, raised over 500 ms and brought down.
void Poser::state1() {
    const bool two = holdsTwoHander();
    if (until(500)) {
        const float k = minK(1.0f, t / 500.0f);
        if (two) {
            RS.y = 0, RS.x = -Rr.x;
            RT.x = k * 120.0f;
            RS.z = 14;
            LS.y = 0, LS.z = 12, LS.x = 2.0f - LS.x;
        } else {
            RS = V(1, 0, 14), LS = V(-1, 0, 14);
            LT.x = -100, RT.x = -100;
        }
        KS = k * -50.0f;
        BT.x = k * -40.0f;
        WA.x = BT.x;
        if (ability == 0x32)
            WA.x = WA.x - minK(360.0f, (t / 500.0f) * 360.0f);
        put<float>(c, STRIDE, 0.0f);
    } else if (until(T)) {
        BT.x = -20, RT.x = -70;
        const float u = (t - 500.0f) / 500.0f;
        WA.x = (1.0f - minK(1.0f, u + u)) * -60.0f;
        if (two) {
            RS.x = -Rr.x, RS.y = 4, RS.z = 0;
            LS.y = 4, LS.z = 0, LS.x = 2.0f - LS.x;
        } else {
            RS = V(1, 3, 8), LS = V(-1, 3, 8);
            LT.x = -150, RT.x = -150;
        }
        rate = 0.1f;
    } else if (two) {
        Rr.x += -8, Rr.y += 4;
        Lr = Rr;
        Lr.x += -2, Lr.z += -1;
        Lm.y += 10, Lm.z += -4;
        RT.z = 20, RT.x = -60;
    } else {
        RS = V(1, 3, 2), LS = V(-1, 3, 2);
        LT.x = -250, RT.x = -250;
    }
    if (category(item(LEFT_ITEM)) == 3 && leftKind() == 0xd)
        LT = V(-250, -90, 0);
}

// 2: a two-handed swing.
void Poser::state2() {
    if (until(d1)) {
        const float k = minK(1.0f, (t / (float)d1) * 1.5f), kk = 1.0f - k;
        BT.x = 10, BT.y = 10;
        WA.x = k * 10.0f + 5.0f;
        BT.z = -60;
        RT = mix(k, V(60, 0, 60), kk, V(90, 0, 90));
        RS = V(0, 0, k * 14.0f);
        LS = RS;
        Lr = Rr;
        Lm.y += 10, Lr.x += 2, Lm.z += -4;
    } else if (until(T)) {
        const float k = minK(1.0f, (t - (float)d1) / (float)d2);
        WA.x = -10, BT.y = 10, BT.z = 60;
        RT = V(-70, 0, -60);
        const float s = nativecrt::sinf((float)((double)k * PI * 0.5)) * 2.0f;
        RS = V(0, s, 0);
        LS = RS;
        Lr = Rr;
        Lr.x += 2, Lr.y += -1;
        rate = 0.04f;
    } else {
        endState();
        Rr.y += 2, Rr.z += 3;
        Lr = Rr;
        Lr.x += -3, Lr.z += -3;
        RT.x = 0, RT.y = 20;
        Lm.y += 10, Lm.z += -4;
    }
    LT = RT;
}

// 3: a sweep with the right arm.
void Poser::state3() {
    bool swinging = true;
    if (until(d1)) {
        WA.z = 0;
        RT = V(-90, 0, 0), RS = V(-5, 8, 0);
        rate = 0.005f;
    } else if (until(d1 + d2 / 2)) {
        WA = V(10, 0, -10), BT = V(30, 0, -30);
        RT = V(0, 0, 90), RS = V(-8, -4, 20);
        rate = 0.005f;
    } else if (until(T)) {
        WA.z = -30, BT = V(0, 20, -60);
        RT = V(70, 0, 90), RS = V(5, -2, 0);
        rate = 0.02f;
    } else {
        swinging = false;
        endState();
        Lr = Rr;
        Lr.x += -2, Lr.z += -1;
        RT.y = 90;
        Lm.y += 10, Lm.z += -4;
        BT.z = -80;
    }
    if (swinging) {
        LS = RS;
        Lr = Rr;
        Lr.x += 2;
        Lm.y += 10, Lm.z += -4;
    }
    LT = RT;
}

// 4: a lunge.
void Poser::state4() {
    const float h = (float)(d1 / 2);
    if (h > t) {
        const float k = minK(1.0f, t / h), kk = 1.0f - k;
        WA.x = k * 5.0f, BT.y = k * 10.0f, BT.z = k * -20.0f;
        RT = mix(k, V(60, 60, 60), kk, V(0, 120, 0));
        Lr = Rr;
        rate = 0.005f;
    } else if (c[CLASS] == 1 && c[SPECIALISATION] == 1) {
        RS = V(2, 7, 4), RT = V(0, -60, 0);
        LS = RS;
        Lr = Rr;
    } else {
        const float k = minK(1.0f, ((t - h) / h) * 1.5f), kk = 1.0f - k;
        WA.x = 0, BT = V(20, 0, 0);
        RT = mix(k, V(30, 0, 0), kk, V(90, 0, 90));
        Rr.z += bobble();
        RS = V(-6, 0, k * 16.0f);
        LS = RS;
        Lr = Rr;
        rate = 0.005f;
    }
    Lr.x += 2;
    Lm.y += 10, Lm.z += -4;
    LT = RT;
}

// 5: both hands up in front, bobbing.
void Poser::state5() {
    RT = V(-90, 0, -40), LT = V(-90, 0, 40);
    Rr.z += bobble(), Lr.z += bobble();
    RS = V(1, -7, 0), LS = V(-1, -7, 0);
    BT.z = -40;
    rate = 0.005f;
}

// 6: the weapon at the ready.
void Poser::state6() {
    if (holdsTwoHander()) {
        RT = V(-40, 60, -90), RS = V(-5, 8, 4);
        BT.z = -55, BT.x = 10, BT.y = 30;
        grip();
        return;
    }
    if (c[SPECIALISATION] == 1) {
        RT = V(-50, -60, 40), LT = V(-90, 90, -50);
        Rr.z += bobble(), Lr.z += bobble();
        RS = V(-3, 7, 3), LS = V(-2, 2, 0);
        BT.z = -20;
    } else {
        RT = V(-90, 0, -40), LT = V(-90, 90, -40);
        Rr.z += bobble(), Lr.z += bobble();
        RS = V(1, -7, 0), LS = V(-1, 0, 1);
        BT.z = -40;
    }
    rate = 0.005f;
}

// 7: changing stance, from the old stance (+0x168) to the new (+0x15c).
void Poser::state7() {
    const float h = (float)(T / 2);
    if (h > t) {
        ST = vec(OLD_STANCE);
        rate = 0.005f;
    } else if (until(T)) {
        const float u = (t - h) / h, k = minK(1.0f, ((u * u) * u) * 15.0f);
        ST = mix(1.0f - k, vec(OLD_STANCE), k, vec(STANCE));
        rate = 0.03f;
    } else
        ST = vec(STANCE);
    RT = V(0, 90, 0);
    if (holdsTwoHander()) {
        Lr = Rr;
        Lr.x += 2;
        Lm.y += 10, Lm.z += -4;
    }
}

// 8: drawing a weapon, by the state clock.
void Poser::state8() {
    if (sprite.leftWeapon) {
        if (until(100))
            BT.z = 80, LT = V(0, 0, 90), RT = V(-45, 45, 0);
        else if (until(500)) {
            BT.z = -90, LT = V(0, 60, 90);
            LS.z += 5;
            RT = V(-90, 0, 45);
        } else
            BT.z = -30, LT = V(0, 100, 60), RT = V(-45, 45, 0);
        return;
    }
    if (until(100))
        BT.z = -80, RT = V(0, 0, 90);
    else if (until(500)) {
        RT = V(0, 60, -90);
        BT.z = 90;
        LS.x += 2, RS.x += 2, RS.y += 3;
        RT.z += 5;
        RS.z += 3, LS.y += 3, LS.z += 3;
    } else {
        Rr.z += 3, Rr.y += 2;
        Lr = Rr;
        Lr.x += -3, Lr.z += -3;
        RT.x = 0;
        Lm.y += 10, Lm.z += -4;
        RT.y = 20;
    }
    Lr = Rr;
    Lr.x += -3, Lr.z += -3;
    LT = RT;
}

// 9: a weapon from the back.
void Poser::state9() {
    LT = V(0, 0, 90);
    if (until(d1)) {
        BT.z = 80, RT = V(-45, 45, 0);
        if (ability == 0x32)
            WA.z = (t / -300.0f) * 360.0f;
    } else
        BT.z = -90, RT = V(-90, 0, 45);
}

// ---------------------------------------------------------------------------------------
// States 0xa - 0x1f.

// 0xa: a weapon from the back, swung, then back to standing.
void Poser::stateA() {
    if ((float)d1 <= t && until(T)) {
        BT.z = -90, LT = V(0, 0, 90), RT = V(-90, 0, 45);
        Lr.x += nativecrt::sinf((t - 600.0f) * 0.1f);
        rate = 0.03f;
    } else if ((float)d1 <= t) {
        ended();
        if (race != 0x60 && race != 0x57)
            standUp(true);
    } else
        state9();
}

void Poser::stateB() {
    if (!(looks & 4))
        BT.z = 60, RT = V(0, -45, -60), LT = V(-45, -45, 0);
}

// 0xf and 0x10: a weapon swing, backhand and forehand.
void Poser::swingF10() {
    const bool forehand = state() == 0x10;
    ST = vec(STANCE);
    if (swingTurn(c) == (forehand ? 1 : 2)) {
        WA.z = clamp01((t - (float)d1) / (float)d2) * -360.0f;
        rate = 0.03f;
    } else
        WA.z = 0;
    if (until(d1)) {
        if (forehand) {
            BT.z = 90, RT = V(0, -90, 0), LT = V(-90, 0, 90);
            RS.y += 6, RS.z += 4;
        } else {
            BT.z = 40, LT = V(0, -90, 30);
            LS.y += -4, LS.z += 4;
            RT = V(0, -90, -120);
            RS.y += -6;
        }
    } else if (until(T)) {
        if (forehand) {
            BT.z = 20, LT = V(-90, -60, 90);
            LS.y += -4, LS.z += 4;
            RT = V(0, -90, -220);
            RS.y += -6;
            Fl.y += -4;
            WS.x = 0.1f;
            Fr.y += 4;
            turnOut = true;
        } else {
            WS.x = -0.1f, BT.z = -90;
            LT = V(-90, -90, -30), RT = V(0, -90, -120);
            RS.y += -6;
        }
    } else {
        ended();
        if (race != 0x60 && race != 0x57)
            standUp(false);
    }
    if (leftKind() == 0xd)
        LT = V(0, 0, 90);
}

// 0x11 and 0x12: a blow, forehand and backhand.
void Poser::blow11_12() {
    const bool backhand = state() == 0x12;
    if (until(d1)) {
        if (backhand) {
            BT.z = -40, LT = V(-90, -90, -30), RT = V(-90, 0, 20);
            RS.y += -6;
        } else {
            BT.z = 40, RT = V(-90, 90, 30), LT = V(-90, 0, -20);
            LS.y += -6;
        }
    } else if (until(T)) {
        if (backhand) {
            BT.z = 90;
            LT = leftKind() == 0xd ? V(0, 0, 40) : V(-40, 0, -30);
            LS = V(-3, 0, 4), RT = V(-90, -90, -90), RS = V(3, -1, 0);
        } else {
            BT.z = -90;
            RT = V(-40, 0, 30), RS = V(3, 0, 4), LT = V(-90, 90, 90), LS = V(-3, -1, 0);
        }
        turnOut = true;
        rate = 0.05f;
    } else {
        ended();
        if (race != 0x60 && race != 0x57)
            standUp(backhand);
    }
}

// 0x13 and 0x14: circling, then a push.
void Poser::states13_14() {
    const bool right = state() == 0x14;
    if (t > (float)d1 && until(T)) {
        if (right) {
            BT.z = 45, WA.z = 45;
            RT = V(0, -90, -90), RS = V(8, 0, 4), LT = V(0, -90, -30), LS = V(-1, -4, -1);
        } else {
            BT.z = -20, WA.z = -20;
            LT = V(0, 90, 40), LS = V(0, 8, 4), RT = V(0, 90, 30), RS = V(1, -4, -1);
        }
        HA.x = -10, BS.y = 3;
    } else
        circle(true, right ? 0.01f : 0.005f);
    rate = 0.03f;
}

// 0x15: drawing and loosing a bow; the other hand shows an arrow (model 0x401).
void Poser::state15() {
    const uint32_t rk = rightKind(), lk = leftKind();
    const bool full = (float)(T * 2) > t, drawing = (float)(T / 2) > t;
    if (!(rk == 6 || rk == 7)) { // the bow is in the left hand
        if (lk == 7)
            LT.y = -90;
        if (full) {
            if (lk == 6)
                LS.x += -7, LS.y += 5;
            else
                LS.x += -3, LS.y += 2;
            LS.z += 4, RS.y += 3;
            LT.z = 70, RT.z = 70;
            RS.x += -10, RS.z += 3;
            BT.z += -70;
            if (drawing) {
                LS.z += -2;
                LT.x = -5;
                RS = V(-5, 1, 0);
                BT.z = -50, LT.z = 70;
                sprite.rightWeapon = nullptr;
            } else if (takesArrow(item(LEFT_ITEM)))
                sprite.rightWeapon = byId(ARROW_MODEL);
        } else {
            LS.x += -1;
            LT.x = -30, RT.x = -30, RT.y = 45;
            if (lk == 7)
                LT.z = 50;
            if (takesArrow(item(LEFT_ITEM)))
                sprite.rightWeapon = byId(ARROW_MODEL);
        }
        return;
    }
    if (rk == 7)
        RT.y = -90;
    if (full) {
        if (rk == 6)
            RS.x += 7, RS.y += 5;
        else
            RS.y += 2, RS.x += 3;
        RS.z += 4, LS.y += 3;
        BT.z = 70, RT.z = -70, LT.z = -70;
        LS.z += 3, LS.x += 10;
        if (drawing) {
            RS.z += -2;
            RT.x = -5;
            LS = V(5, 1, 0);
            BT.z = 50, RT.z = -70;
            sprite.leftWeapon = nullptr;
        } else if (takesArrow(item(RIGHT_ITEM)))
            sprite.leftWeapon = byId(ARROW_MODEL);
    } else {
        RS.x += 1;
        RT.x = -30, LT.x = -30, LT.y = -45;
        if (takesArrow(item(RIGHT_ITEM)))
            sprite.leftWeapon = byId(ARROW_MODEL);
    }
}

// 0x16: aiming.
void Poser::state16() {
    const uint32_t rk = rightKind();
    if (rk == 6 || rk == 7) {
        RS.y += 3, LS.x += 10, RS.x += 4;
        RT.y = -90, BT.z = 60;
        RS.z += 2;
        RT.z = -60, LT.z = -60;
        LS.y += 2, LS.z += 1;
        if (takesArrow(item(RIGHT_ITEM)))
            sprite.leftWeapon = byId(ARROW_MODEL);
        if (until(300)) {
            RS.z += -1, RT.x += 20;
            RT.y = -70;
        }
    } else {
        LS.y += 3, LS.x += -4, RS.x += -10;
        LT.y = -90;
        RS.y += 2, LS.z += 2;
        LT.z = 60, RT.z = 60;
        RS.z += 1;
        BT.z = -60;
        if (takesArrow(item(LEFT_ITEM)))
            sprite.rightWeapon = byId(ARROW_MODEL);
        if (until(300)) {
            LS.z += -1, LT.x += 20;
            LT.y = -110;
        }
    }
}

// 0x17, 0x1a, 0x1c: hands together in front, circling.
void Poser::states17_1a_1c() {
    const int st = state();
    if (st == 0x1c || until(d1))
        handsCircling();
    else if (st == 0x17)
        BT.z = 60, RT = V(0, -45, -60), LT = V(-45, -45, 0);
    else if (until(T)) {
        BT.z = 90, RT = V(-90, 0, -90), LT = V(-45, -45, 0), RS = V(3, 0, 1);
        rate = 0.03f;
    } else
        RT = V(-60, 0, 0), RS = V(3, 0, 1);
}

// 0x18 and 0x19: circling, then a push to the side (0x19 left, 0x18 right).
void Poser::states18_19() {
    const bool left = state() == 0x19;
    if (until(d1)) {
        RS.y += 4, LS.y += 4;
        RS.x += nativecrt::sinf(t * 0.04f) * 0.5f;
        RS.z += nativecrt::cosf(t * 0.05f) * 0.5f;
        LS.x += nativecrt::sinf(t * 0.04f) * 0.5f;
        BT.z = left ? -20.0f : 20.0f;
        LS.z += nativecrt::cosf(t * 0.05f) * 0.5f;
    } else if (until(T)) {
        turnOut = true;
        if (left) {
            WS.x = -0.5f, BT.z = 60, WA.z = 30;
            RT = V(0, -45, -60), LT = V(-45, -45, 0), RS = V(2, 0, 0);
        } else {
            BT.z = -60, WS.x = 0.5f, WA.z = -30;
            LT = V(0, 45, 60), RT = V(-45, 45, 0), LS = V(-2, 0, 0);
        }
    }
}

// 0x1b: eating or drinking.
void Poser::state1b() { bobWhileEating(); }

// 0x1d: a spinning swing.
void Poser::state1d() {
    if (until(d1))
        BT.z = -80, WA.z = 0, RT = V(-90, 90, -90), LT = V(20, 90, 90);
    else if (until(d1 + d2)) {
        BT.z = -90;
        WA.z = ((t - (float)d1) * 360.0f) / (float)d2;
        LT = V(0, 100, 90), RT = V(0, 90, 0);
        rate = 0.2f;
    } else
        BT.z = 80, WA.z = 0, LT = V(90, 90, 270), RT = V(0, 90, 90);
}

// 0x1e: arms crossed, then flung wide.
void Poser::state1e() {
    if (until(d1)) {
        RT = V(-80, -90, 60), LT = V(-80, 90, -60);
        RS.y += 3, RS.x += -5, LS.x += 5;
    } else if (until(T)) {
        BT = V(0, 0, -30);
        LS.y += -5, RS.y += -5;
        LT = V(-90, 90, 110), RT = V(-90, -90, -110);
        WS.y = -0.5f;
        rate = 0.04f;
    } else {
        ended();
        standUp(false);
    }
}

// 0x1f: a two-handed overhead swing.
void Poser::state1f() {
    if (until(d1)) {
        const float k = t / (float)d1;
        RT = V(-80, 90, -90), RS = V(-10, 8, 8);
        BT.z = -45;
        WA.z = k * -20.0f;
        WS.x = k * 0.0f;
        if (swingTurn(c) == 2 && until(d1 + d2 / 2))
            WA.z = WA.z + k * 360.0f;
        rate = 0.04f;
    } else if (until(d1 + d2 + d3)) {
        const float k = minK(1.0f, ((t - (float)d1) * 1.2f) / (float)d2), kk = 1.0f - k;
        WS.x = kk * 0.0f - k * 0.8f;
        RT = mix(kk, V(-90, 90, 48), k, V(-110, 60, 0));
        RS = mix(kk, V(-10, 8, 8), k, V(-12, 0, -1));
        HA.x = -10;
        BT.z = k * 90.0f - kk * 45.0f;
        WA.z = k * 30.0f;
        splay = k * 10.0f;
        rate = 0.05f;
    } else {
        RS = V(2, 0, 0), RT = V(0, 0, 0), BT.z = -30;
        rate = 0.03f;
    }
    grip();
}

// ---------------------------------------------------------------------------------------
// States 0x20 - 0x3f.

// 0x20, 0x31, 0x32: two-handed swings.
void Poser::swings20_31_32() {
    const int st = state();
    const bool first = st == 0x31;
    if (until(d1)) {
        const float k = t / (float)d1;
        if (first) {
            RT = V(-40, 60, -90), RS = V(-5, 8, 4);
            BT.x = 10, BT.z = -55, BT.y = 30;
            WA.z = k * -20.0f;
            WS.x = k * 0.0f;
        } else {
            RT = st == 0x20 ? V(0, -90, 0) : V(30, -60, 0);
            RS = st == 0x20 ? V(-8, 6, 6) : V(-14, 5, 5);
            WS.x = -0.0f;
            HA.x = -10, HA.z = 30;
            BT.z = 45, WA.z = 20;
        }
        if (swingTurn(c) == 2 && until(d1 + d2 / 2))
            WA.z = first ? WA.z + k * 360.0f : WA.z - k * 360.0f;
        if (st == 0x20)
            rate = 0.05f;
    } else if (until(first ? d1 + d2 + d3 : T)) {
        const float k = minK(1.0f, ((t - (float)d1) * 1.2f) / (float)d2), kk = 1.0f - k;
        if (first) {
            splay = k;
            WS.x = kk * 0.0f - k * 0.8f;
            RT = mix(k, V(-60, 60, 60), kk, V(-60, 60, 60));
            RS = mix(k, V(-12, 4, 2), kk, V(-10, 8, 4));
            BT.x = -20, HA.x = -10;
            BT.z = k * 50.0f - kk * 45.0f;
            WA.z = k * 30.0f;
            splay = splay * 10.0f;
        } else {
            HA.z = -30;
            WS.x = kk * -0.0f + k * 0.8f;
            RT = mix(k, V(0, -90, -130), kk, V(0, -90, 20));
            RS = mix(k, V(0, 2, 0), kk, V(-8, 6, 6));
            BT.z = kk * 45.0f - k * 45.0f;
            WA.x = k * -20.0f;
            WA.z = k * -30.0f;
            splay = k * 10.0f;
        }
        rate = 0.05f;
    } else {
        RS = V(2, 0, 0), RT = V(0, 0, 0), BT.z = -30;
        rate = 0.03f;
    }
    grip();
}

// 0x21: a two-handed swing from the side.
void Poser::state21() {
    if (until(d1)) {
        const float k = t / (float)d1;
        RS = V(2, 2, 14), RT = V(50, 0, 0);
        BT.z = -45;
        WA.z = k * -20.0f;
        WS.x = k * 0.5f;
        rate = 0.04f;
    } else if (until(d1 + d2 + d3)) {
        const float u = t - (float)d1;
        const float k = minK(1.0f, (u + u) / (float)d2), kk = 1.0f - k;
        WA.x = kk * 20.0f - k * 10.0f;
        RT = mix(kk, V(90, 0, 45), k, V(-90, 0, -45));
        RS = mix(kk, V(2, 2, 14), k, V(0, 9, 0));
        BT.z = k * 45.0f - kk * 45.0f;
        rate = 0.05f;
    } else {
        RS = V(2, 0, 0), RT = V(0, 0, 0), BT.z = -30;
        rate = 0.03f;
    }
    grip();
}

// 0x22: a two-handed thrust.
void Poser::state22() {
    if (until(d1)) {
        RS = V(4, 2, 8), RT = V(-90, -180, 90);
        BT.z = -90, WS.y = -1;
        rate = 0.02f;
    } else if (until(d1 + d2)) {
        RS = V(6, 0, 0);
        WS.y = 0.5f, BT.z = 90;
        RT = V(-90, 0, -90);
        turnOut = true;
        rate = 0.05f;
    } else {
        RS = V(2, 0, 0), RT = V(0, 0, 0), BT.z = -30;
        rate = 0.03f;
    }
    grip();
}

void Poser::state23() { RT = V(-90, -90, 80), BT.z = -80; }

// 0x24: a two-handed blow from above.
void Poser::state24() {
    if (until(d1)) {
        RT = V(-90, 0, 0), LT = V(-90, 0, 40);
        RS = V(1, -7, 0), LS = V(-1, -7, 0);
        BT.z = -40, WS.y = -0.5f;
        turnOut = true;
        rate = 0.005f;
    } else if (until(T)) {
        RT = V(-90, -60, -30), LT = V(-90, 60, -50);
        RS = V(3, 5, 2), LS = V(6, 8, 2);
        BT.z = 40;
        turnOut = true;
        rate = 0.05f;
    } else {
        ended();
        if (race != 0x60 && race != 0x57)
            standUp(false);
    }
}

// 0x26: a punch.
void Poser::state26() {
    if (until(d1))
        RT = V(-90, -90, 80), BT.z = -80;
    else if (until(T)) {
        RT = V(-90, 0, -90);
        Rr.x += 4, Fl.y += -4;
        BT.z = 90;
        Fr.y += 4;
        turnOut = true;
        rate = 0.05f;
    } else {
        endState();
        Rr.y += 4, Rr.x += -8;
        Lm.y += 10, Lm.z += -4;
        RT.x = -60, RT.z = 20, BT.z = -20;
    }
    Lr = Rr;
    Lm.y += 10, Lr.x += 2, Lm.z += -4;
    LT = RT;
}

// 0x27: a head butt to the side.
void Poser::state27() {
    if (until(d1 + d2 / 2))
        HA = V(-30, 10, 90), BT = V(0, 0, 60), HS = V(-3, -2, 0);
    else if (until(T))
        HA = V(30, -10, -30), BT = V(10, 0, -20), HS = V(3, 0, 2);
}

// 0x28: a head butt forward.
void Poser::state28() {
    if (until(d1 + d2 / 2)) {
        HA = V(-70, 0, 0), BT = V(-10, 0, 0), HS = V(0, -3, -1);
        LT.x = 20, RT.x = 20;
    } else if (until(T)) {
        HA = V(50, 0, 0);
        TAx = -10, TAz = 0;
        BT = V(20, 0, 0), BS = V(0, -3, 1), HS = V(0, 0, 2);
        LT.x = -20, RT.x = -20;
    }
}

// 0x29: a tail sweep.
void Poser::state29() {
    if (until(d1)) {
        HA.z = -60, TAz = -70, TAx = 0, WA.z = 90, HA.x = 0, HA.y = 10;
        rate = 0.002f;
    } else if (until(T))
        WA.z = 200, TAz = 70, TAx = 0, HA = V(0, -20, 60), HS = V(-3, 0, -2);
}

// 0x2a: curling into a ball and back.
void Poser::state2a() {
    if (until(d1))
        gather(t / (float)d1);
    else if (until(d1 + d2)) {
        sprite.head = sprite.hands = sprite.feet = sprite.tail = nullptr;
        BS = V(0, 0, -3);
    } else if (until(T))
        gather(1.0f - ((t - (float)d1) - (float)d2) / (float)d3);
}

void Poser::state2b() {
    if (until(d1 + d2 / 2)) {
        HA.y = 10, HA.z = 30;
        BT = V(-10, 0, 0), HS = V(-3, 0, -1);
        LT.x = 20, RT.x = 20, HA.x = -10;
        rate = 0.005f;
    } else if (until(T)) {
        HA.x = 50, HA.y = 30, HA.z = 0;
        if (until(d1 + d2))
            HA.y = -30;
        RT.x = -60, TAx = -20, TAz = 0;
        BT = V(30, 0, 0), BS = V(0, -1, 2), HS = V(0, 0, 2);
        LT.x = -20;
        rate = 0.005f;
    }
}

void Poser::state2c() {
    if (until(d1 + d2 / 2)) {
        BT.x = 50, HS.y = -6, TAx = -50, TAz = 0;
        HA = V(-20, 0, 0);
        BT.y = 0, BT.z = 0;
        BS = V(0, -2, 2);
        HS.x = 0, HS.z = -2;
        LT.x = 20, RT.x = 30;
    } else if (until(T)) {
        BT = V(0, 0, 0), HS = V(0, 0, 1);
        LT.x = 0, RT.x = 0;
        HA.y = 0, HA.z = 0, HA.x = -10;
        rate = 0.02f;
    }
}

// 0x2d and 0x50: a bite or peck in four beats.
void Poser::bite2d_50() {
    auto open = [this] {
        TAx = 30, TAz = 10;
        BS.x = 0, BT.y = 0, BT.z = 0, BT.x = -20;
        HA.x = 10, HA.y = 0;
        HS = V(0, 1, 1);
        LT.x = 20, RT.x = 20, LS.z = 2, RS.z = 2;
    };
    auto lean = [this] {
        TAx = -30, TAz = -10;
        HS = V(0, -2, 2), BS = V(0, 1, 1);
        BT.z = 0;
        HA = V(-30, 0, 0);
    };
    if (until(d1)) {
        HA.z = 0, BS.y = -1, BS.z = 0;
        open();
    } else if (until(d1 + d2 / 3)) {
        BT.x = 50, TAx = -30, TAz = 0;
        HA = V(-30, 0, 0);
        BT.y = 0, BT.z = 0;
        BS = V(0, 0, 1), HS = V(0, -4, 2);
        LT.x = 60, RT.x = 70;
    } else if (until(d1 + (d2 + d2) / 3)) {
        BT.x = 40, BT.y = 0, LT.x = 70, RT.x = -30, RS = V(0, 3, -2);
        lean();
    } else if (until(d1 + d2)) {
        BT.x = 10, BT.y = 0, LT.x = -30, RT.x = -40, LS = V(0, 3, -2);
        lean();
    } else if (until(T)) {
        HA.z = -10, BS.y = 1, BS.z = 0;
        open();
    }
}

// 0x2e: a somersault forward.
void Poser::state2e() {
    if (until(d1))
        TAx = -60, BT.x = -10, HA.x = 10;
    else if (until(d1 + d2)) {
        LT.x = 80, RT.x = 80;
        HS.x = 0, HS.y = -3;
        HA.x = -60;
        WA.x = ((t - (float)d1) / (float)d2) * -360.0f;
        HS.z = 0;
    } else if (until(T))
        BT.x = 20, HA.x = -10, HS = V(0, -2, 0);
}

// 0x2f: a roll to one side.
void Poser::state2f() {
    if (until(d1))
        BT.z = -10, BT.y = 30, LT.y = -30, RT.y = -30, HA.z = 60;
    else if (until(d1 + d2)) {
        LT.y = 80, RT.y = 80;
        WA.y = ((t - (float)d1) / (float)d2) * -360.0f;
    } else if (until(T))
        BT.y = -30, LT.y = 30, RT.y = 30;
}

// 0x30: a roll to the other side.
void Poser::state30() {
    if (until(d1))
        HA.z = -60, BT.z = 10, BT.y = -30, LT.y = 30, RT.y = 30;
    else if (until(d1 + d2)) {
        LT.y = -80, RT.y = -80;
        WA.y = ((t - (float)d1) / (float)d2) * 360.0f;
    } else if (until(T))
        BT.y = 30, LT.y = -30, RT.y = -30;
}

// 0x33: arms up, a turn, then hands circling.
void Poser::state33() {
    RT = V(60, -20, 20), RS = V(0, 2, 4), LT = V(60, 20, -20), LS = V(0, 2, 4);
    if (until(d1)) {
        BT.z = 50;
        LT.x = -90, LT.z = 90, RT.x = -90;
        LS = V(-4, -4, -2), RS = V(0, 4, 1);
    } else if (until(d1 + d2)) {
        LT.x = -90, LT.z = 90, RT.x = -90;
        const float k = clamp01(((t - (float)d1) / (float)d2) * 1.5f);
        WA.z = k * -270.0f, WA.y = k * -40.0f;
        splay = k * 40.0f;
        BS.z += 4;
        LS = V(-4, -4, -2), RS = V(0, 4, 1);
        HA.z = -60, HA.x = -30;
        rate = 0.03f;
    } else
        circle(false, 0.005f);
}

// 0x34 and 0x35: using an item.
void Poser::using34_35() {
    if (state() == 0x35) {
        WS.z = -0.14f, BT.x = 10;
        LS = V(0, 4, 0), RS = V(0, 4, 0), HS = V(0, 2, -2);
    }
    if ((float)U > c6c) {
        HA.y = 0, HA.z = 0;
        BT = V(10, 0, 0), WA = V(10, 0, 0);
        LS = V(1, 7, 8);
        HA.x = nativecrt::cosf(t * 0.02f) * 5.0f + 45.0f;
        RS = V(-1, 7, 8);
        LT = V(100, 0, 0), RT = V(100, 0, 0);
    }
}

void Poser::state36() { BT = V(-30, 0, 0), LS = V(4, 6, 6), RS = V(-4, 6, 6), BS.y = 3; }

// 0x37: swimming.
void Poser::state37() {
    if (length2(vec(INTENT)) > 0.0f) {
        LS.y += 4, RS.y += 4;
        WA.x = 20, BT.x = -10, HA.x = -20, LT.x = -20, RT.x = -20;
        if (!(looks & 1)) {
            Fr.y += 4, Fl.y += 4, Fl.z += 3, Fr.z += 3;
            KS = -40;
        }
    } else
        k4 = Rr.z; // creature-pose.md 5.3
}

// 0x38 and 0x39: sitting.
void Poser::sitting38_39() {
    const float sit = (f32(DEPTH) - f32(HEIGHT)) * 0.5f;
    if (looks & 1)
        WA = V(0, 90, 0), WS.z = (f32(WIDTH) - f32(HEIGHT)) * 0.5f;
    else if (state() == 0x38) {
        WA = V(-90, 0, 0), HS = V(0, -1, 0), HA.x = 20;
        LS = V(0, 0, 3), RS = V(0, 0, 3);
        WS.z = sit - 0.15f;
    } else
        WA = V(90, 0, 0), WS.z = sit - 0.1f;
}

void Poser::state3a() {
    WA = V(90, 0, 0);
    Fl.y += 4, Fr.y += 4;
    LS = V(0, -4, 11), RS = V(0, -4, 11);
    WS.z = (f32(DEPTH) - f32(HEIGHT)) * 0.5f;
}

// 0x3b and 0x3c: hanging and climbing.
void Poser::hanging3b_3c() {
    KS = -30;
    if (state() == 0x3b) {
        Lr = V(-1, -3, -3), Rr = V(1, -3, -3);
        WS.z = 0.3f;
        Fr.y += 4, Fl.y += 4;
    } else {
        WA.x = 178;
        Lr = V(-4, 0, 12), Rr = V(4, 0, 12);
        Fr.y += 4, Fl.y += 4;
        BT.y = nativecrt::cosf(t * 0.005f) * 10.0f;
    }
}

// 0x3d: arms raised.
void Poser::state3d() {
    if (ability != 0x76)
        WA.x += 20, BT.x += 30;
    LS = V(-2, 0, 0), RS = V(2, 0, 0);
    LT = V(-10, -10, 90), RT = V(-10, 10, -90);
}

// 0x3e: lying down.
void Poser::state3e() {
    HS.x = 0;
    if (looks & 1) {
        HS.y += 1;
        BT.x = 30;
        LS = V(-1, -7, 0);
        Fl.z += 2, Fr.z += 2;
        RS = V(1, -7, 0);
        LT.x = -100, RT.x = -100;
        HS.z = -4;
    } else {
        const float k = ((f32(CHEST_PLACE + 8) + 5.0f) / 140.0f + 0.075f) * f32(HEIGHT);
        HS.y = 2, HS.z = -2;
        BT.x = 10;
        LS = V(0, 4, 0);
        WS.z = -k;
        RS = V(0, 4, 0);
    }
}

// 0x3f and 0x40: riding a creature, or a boat or similar.
void Poser::riding3f_40() {
    if (state() == 0x3f) {
        if (const uint8_t *mount = call.mount) {
            WS.z = WS.z + (get<float>(mount, HEIGHT) * 0.75f + liftL * 0.3f);
            WS.y += -0.25f;
            const int mountRace = get<int32_t>(mount, RACE);
            const Vec3 o = mountOffset(mountRace);
            WS.x += o.x, WS.y += o.y, WS.z += o.z;
            if (mountRace == 0x16 || mountRace == 0x17)
                WS.z += -0.75f;
            DR = -WS.z;
        }
        HS.y = 2, HS.z = -2;
    } else {
        WS.z += (moveFlags & 1) ? 0.85f : 1.25f;
        WA.z = -50;
        HS.y = 1, HS.z = 0;
        HA = V(0, 0, 30);
    }
    RT.z = -60, HS.x = 0;
    RS = V(-4, 6, -3), LS = V(4, 6, -3);
    BT.x = 20;
    LT = V(60, 90, 60);
    RT.x = 60, RT.y = -90;
}

// ---------------------------------------------------------------------------------------
// States 0x41 - 0x66.

// 0x41: a shield bash.
void Poser::state41() {
    if (until(T))
        BT.z = 0, LT = V(0, -90, 0), RT = V(0, 90, 0);
    else
        BT.z = 80, WA.z = 0, LT = V(90, 90, 270), RT = V(0, 90, 90);
}

// 0x42, 0x43, 0x44: stirring, a push, arms flung up.
void Poser::stirring42_44() {
    const int st = state();
    if (st == 0x44 || until(d1)) {
        BT.x = 20, HA.x = 20;
        const float u = t * 0.005f;
        LS = V(nativecrt::cosf(u) * 3.0f - 1.0f, 5, nativecrt::sinf(u) * 5.0f + 5.0f);
        const float v = (float)((double)(t * 0.005f) + PI);
        RS = V(nativecrt::cosf(v) * 3.0f + 1.0f, 5, nativecrt::sinf(v) * 5.0f + 5.0f);
    } else if (until(d1 + d2)) {
        BT.x = -20, BT.z = 10;
        LS = V(2, 4, 0), RS = V(-2, 6, 0), LT = V(0, 45, 0), RT = V(0, -45, 0);
    } else if (until(T)) {
        LS = V(-3, -6, 0), RS = V(3, -6, 0), LT = V(-90, 0, 130), RT = V(-90, 0, -130);
        rate = 0.03f;
    }
    if (st == 0x43 && until(T))
        bobWhileEating();
}

// 0x45: a flurry of alternating blows.
void Poser::state45() {
    if (!until(d1 + d2 + 400))
        return;
    const int n = (int)t - d1 + 100;
    if (!((float)d1 >= t) && (n / 500) % 2 != 1) {
        rate = 0.03f;
        if ((n / 1000) & 1)
            LS = V(2, -2, 12), RS = V(0, 0, -5), BT = V(-10, 0, 20), WA = V(-5, 0, 10);
        else
            LS = V(0, 0, -5), RS = V(-2, -2, 12), BT = V(-10, 0, -20), WA = V(-5, 0, -10);
    } else {
        LS = V(2, -2, 12), RS = V(-2, -2, 12);
        BT.x = 20, WA.x = 10;
    }
}

// 0x47: by the ability clock.
void Poser::state47() {
    if ((float)d1 > c6c) {
        BT.z = 60, WA.z = 60, WA.x = -25;
        RS = V(-6, -3, 0), LS = V(-1, -4, -1);
        HA.x = -10;
    } else if ((float)T > c6c) {
        BT.z = -30, WA.z = -30, WA.x = 10;
        RS = V(3, -4, 0), LS = V(0, 2, -1);
    }
}

// 0x48: a throw; the weapon passes to the other hand and flies off.
void Poser::state48() {
    if (f32(THROW) > 0) {
        if (until(d1)) {
            BT.z = -90, RS = V(2, -2, 1), RT = V(0, 10, 0);
            sprite.leftWeapon = sprite.rightWeapon;
        } else if (until(d1 + 300)) {
            BT.z = 10, LS = V(-2, -2, 1), LT = V(0, -10, 0);
            sprite.leftWeapon = sprite.rightWeapon;
            sprite.rightWeapon = nullptr;
        } else if (until(T)) {
            BT.z = 90;
            sprite.leftWeapon = sprite.rightWeapon = nullptr;
        }
        rate = 0.03f;
        if (t > (float)d1 && until(T)) {
            WA.z = ((t - (float)d1) * 360.0f) / 200.0f;
            if (WA.z > 700.0f)
                WA.z = 700;
        }
    } else if (until(d1))
        BT.z = -90, RS = V(2, -2, 1), RT = V(0, 10, 0);
    else if (until(T)) {
        BT.z = 90;
        sprite.rightWeapon = nullptr;
    }
}

// 0x49: holding something out.
void Poser::state49() {
    BT.z = f32(HOLD_OUT) * -90.0f;
    RS = V(2, -2, 1), RT = V(0, 10, 0);
    Rr.z += bobble(), Lr.z += bobble();
    if (f32(HOLD_OUT) > 0)
        sprite.leftWeapon = sprite.rightWeapon;
}

// 0x4a: arms out in front, then apart.
void Poser::state4a() {
    if (until(d1))
        BT.x = -20, LT = V(-90, 0, 0), RT = V(-90, 0, 0), RS = V(1, -2, 0), LS = V(-1, -2, 0);
    else
        LT = V(0, 45, 0), BT.x = 0, RT = V(0, -45, 0), RS = V(-2, 5, 0), LS = V(2, 6, 0);
}

// 0x4b: a whirl.
void Poser::state4b() {
    if (until(d1 + d2)) {
        WA.z = (t * -360.0f) / (float)(d1 + d2);
        RT = V(-90, -90, -90), RS = V(4, 3, 0);
        BT.z = (t * -60.0f) / (float)(d1 + d2);
        LT = V(0, 0, 90);
        rate = 0.03f;
    } else
        LT = V(0, 0, 90), RT = V(-60, -60, -60);
}

void Poser::state4c() {
    if (until(d1))
        WA.x = -30;
    else if (until(d1 + d2))
        WA.x = 120;
}

// 0x4d: swinging model 0xb04, by the ability clock.
void Poser::state4d() {
    if ((float)d1 > c6c) {
        RS = V(-3, -2, 10), LS = V(3, -2, 10);
        sprite.leftWeapon = byId(SWUNG_MODEL);
        heldScale = heldScale * 1.5f;
        BT.x = 40, WA.x = 10;
    } else if ((float)(d1 + d2) > c6c)
        RS = V(0, 5, 0), LS = V(0, 5, 0), BT.x = -20, WA.x = -40;
    else
        RS = V(2, -1, 0), LS = V(-2, -1, 0);
}

// 0x4e and 0x4f: eating and drinking.
void Poser::eating4e_4f() {
    if (until(200))
        BT.x = -20, HA.x = -10, LS = V(4, 5, 0), RS = V(-4, 5, 0);
    else if (until(5000)) {
        BT.x = 20, HA.x = 30, LS = V(0, 0, 16), RS = V(0, 0, 16);
        if (state() == 0x4f)
            bobWhileEating();
    }
}

void Poser::state51() { BT.x = 20, LS = V(0, 0, 12), RS = V(0, 0, 12); }

void Poser::state52() {
    if (until(1000))
        BT.x = -30, LS = V(0, 8, 3), RS = V(0, 8, 3);
}

void Poser::state54() { WS.z = f32(HEIGHT) * -0.65f; }

void Poser::state55() {
    if (until(T))
        BT.x = -30, RT = V(-90, 0, -60), LT = V(-90, 0, 60), RS = V(3, -3, 1), LS = V(-3, -3, 1);
    else
        RT = V(-60, 0, 0), RS = V(3, 0, 1);
}

// 0x56: beating arms.
void Poser::state56() {
    if (!(t > (float)d1))
        return;
    const float s = nativecrt::sinf((float)((double)(((t - (float)d1) / (float)d2) * 32.0f) * PI));
    BT.z = -50.0f - s * 40.0f;
    RS.x = s * 8.0f - 3.0f, RS.y = 7.0f - s * 5.0f;
    LS.x = -5.0f - s * 8.0f, LS.y = 5.0f - s * 5.0f;
    WA.z = -20.0f - s * 10.0f;
    if (rightKind() != 4)
        RT.x = -90, LT.x = -90;
    RT.z = -BT.z, LT.z = -BT.z;
    WA.x = -50;
    rate = 0.02f;
}

// 0x57 and 0x58: circling, then a push.
void Poser::circling57_58() {
    const int st = state();
    if (st == 0x58 || until(d1)) {
        BT.z = -60, WA.z = 0;
        RT = V(60, 40, 20);
        const float u = t * 0.01f;
        RS = V(0, nativecrt::sinf(u) + 2.0f, nativecrt::cosf(u));
        Rr.z += bobble();
        LT = V(60, 20, -20);
        const float v = (float)((double)(t * 0.01f) + HALF_PI);
        LS = V(0, nativecrt::sinf(v) + 2.0f, nativecrt::cosf(v) + 4.0f);
        BS.z += nativecrt::sinf(t * 0.01f) * 0.5f;
    } else if (until(T)) {
        BT.z = 45, WA.z = 45;
        RT = V(0, -60, -90), RS = V(8, 0, 4), LT = V(0, -90, -30), LS = V(-1, -4, -1);
        HA.x = -10, BS.y = 3;
        turnOut = true;
    } else
        circle(false, 0);
    if (st == 0x57)
        rate = 0.03f;
}

void Poser::state59() {
    if (until(d1 + d2 / 2)) {
        BT = V(-10, 0, 0), LS = V(-3, 2, 0), RS = V(3, 2, 0);
        HA.y = 0, HA.z = 0, HA.x = -10;
        rate = 0.005f;
    } else if (until(T)) {
        HA = V(50, 0, 0), BT = V(30, 0, 0), LS = V(-4, 3, 5), RS = V(4, 3, 5);
        LT.y = -10, RT.y = 10;
        rate = 0.005f;
    }
}

// 0x5a - 0x60 and 0x62: swaying while busy, by the ability clock.
void Poser::busy5a_62() {
    const int st = state();
    if (!((st == 0x62 ? 1000.0f : 5000.0f) > c6c))
        return;
    auto swaying = [&](float headRate, float headSwing) { // the head turns gently
        HA = V(0, nativecrt::cosf(t * headRate) * headSwing, 0);
    };
    if (st == 0x5b || st == 0x5e || st == 0x5f || st == 0x60) { // common C
        swaying(0.002f, 5);
        BT = V(nativecrt::cosf(t * 0.0015f) * 2.0f, nativecrt::sinf(t * 0.002f) * 10.0f, 0);
        WA = V(0, 0, 0);
    }
    const float u = t * 0.0075f;
    switch (st) {
    case 0x5a: {
        swaying(0.003f, 10);
        const float w = t * 0.002f;
        BT = V(nativecrt::cosf(w) * 5.0f, nativecrt::sinf(w) * 5.0f, 0);
        WA = V(0, 0, 0);
        LS = V(12, 5, 5), RS = V(7, 5, 5);
        break;
    }
    case 0x5d: {
        swaying(0.003f, 10);
        const float z = nativecrt::sinf(t * 0.003f) * 15.0f;
        const float w = t * 0.002f;
        BT = V(nativecrt::cosf(w) * 5.0f, nativecrt::sinf(w) * 5.0f, z);
        WA = V(0, 0, 0);
        LS = V(4, 9, 3.5f), RS = V(-4, 13, 3.5f);
        break;
    }
    case 0x5b:
        LS = V(-5, 7, 4);
        RS = V(nativecrt::cosf(u) * 2.0f - 7.0f, nativecrt::sinf(u) * 2.0f + 10.0f, 3);
        break;
    case 0x5e:
        RS = V(nativecrt::cosf(u) * 2.0f - 5.0f, nativecrt::sinf(u) * 2.0f + 10.0f, 5);
        RT.y = nativecrt::sinf(t * 0.01f) * 40.0f;
        break;
    case 0x5f:
        LS = V(-10, 6, 1);
        RS = V(nativecrt::cosf(u) - 10.0f, 9, nativecrt::sinf(u) * 2.0f + 1.0f);
        break;
    case 0x60:
        RS = V(-5, 10, 6);
        break;
    case 0x5c:
        swaying(0.002f, 5);
        BT = V(nativecrt::cosf(t * 0.0015f) * 2.0f, nativecrt::sinf(t * 0.002f) * 3.0f,
               nativecrt::sinf(t * 0.0015f) * 15.0f);
        WA = V(0, 0, 0);
        LS = V(7, 21, 3);
        RS = V(nativecrt::cosf(t * 0.0075f) * 0.5f + 5.0f, 11, 4);
        break;
    case 0x62:
        swaying(0.003f, 10);
        BT = V(10, 0, 20);
        RT.z = -20;
        WA = V(0, 0, 0);
        LS = V(-3, -2, 0), RS = V(0, 8, 2.5f);
        break;
    }
}

// 0x61: leaning back, then the head thrown back (the sparks are drawn apart).
void Poser::state61() {
    if (t > 2000.0f) {
        const float u = t * 0.01f;
        RS = V(1, 0, nativecrt::cosf(u) * 5.0f);
        LS = V(-1, 0, nativecrt::sinf(t * 0.01f) * 5.0f);
        const float w = t * 0.003f;
        HA = V(-30, nativecrt::cosf(w) * 5.0f, nativecrt::sinf(w) * 10.0f);
        BT = V(-15, nativecrt::sinf(t * 0.002f) * 10.0f, 0);
    } else {
        HA = V(40, nativecrt::cosf(t * 0.003f) * 5.0f, nativecrt::sinf(t * 0.005f) * 10.0f);
        BT = V(10, nativecrt::sinf(t * 0.005f) * 10.0f, 0);
        RS = V(-4, 2, 14);
    }
}

// 0x63: playing an instrument.
void Poser::state63() {
    if (!until(2000))
        return;
    BS.z += -3, BS.y += 2;
    BT.x = -40, HA.x = -30;
    BT.z = nativecrt::cosf(t * 0.01f) * 5.0f;
    LS = V(4, 8, nativecrt::cosf(t * 0.02f) * 2.0f + 6.0f);
    RS = V(-4, 8, nativecrt::cosf((float)((double)(t * 0.02f) + PI)) * 2.0f + 6.0f);
}

// 0x64, 0x65, 0x66: swaying while the ability lasts.
void Poser::swaying64_66() {
    if (!until(T))
        return;
    const int st = state();
    if (st == 0x64) {
        BT = V(10, nativecrt::sinf(t * 0.0075f) * 10.0f, 0);
        RS = V(nativecrt::sinf((float)(((double)(BT.y * 2.0f) * PI) / 180.0)) * 8.0f, 2, 15);
    } else if (st == 0x65) {
        BT = V(nativecrt::cosf(t * 0.003f) * 5.0f - 25.0f, nativecrt::sinf(t * 0.005f) * 6.0f, 0);
        BS.y = 5;
        RS = V(nativecrt::sinf(t * 0.01f) * 4.0f, 6, 10);
        RT.x = 20;
    } else {
        const float y = nativecrt::sinf(t * 0.005f) * 30.0f, z = nativecrt::sinf(t * 0.002f) * 10.0f;
        BT = V(nativecrt::cosf(t * 0.003f) * 10.0f, y, z);
        BS.x = nativecrt::sinf((float)(((double)y * PI) / 180.0)) * 5.0f;
        RS = V(nativecrt::sinf(t * 0.01f) * 4.0f + 2.0f, 6, 5);
        LS = V(nativecrt::sinf(t * 0.01f) * 4.0f - 2.0f, 6, 5);
        KS = nativecrt::cosf(t * 0.0045f) * 4.0f - 4.0f;
        WS.z = 1.0f - nativecrt::cosf((float)(((double)KS * PI) / 180.0));
    }
}

// ---------------------------------------------------------------------------------------
// The jump table: each pose state's entry. States not listed (0, 0xc, 0xd, 0xe, 0x25, 0x46,
// 0x53 and above 0x66) set nothing.

const Poser::StateRow Poser::STATES[] = {
    {0x01, &Poser::state1},          {0x02, &Poser::state2},          {0x03, &Poser::state3},
    {0x04, &Poser::state4},          {0x05, &Poser::state5},          {0x06, &Poser::state6},
    {0x07, &Poser::state7},          {0x08, &Poser::state8},          {0x09, &Poser::state9},
    {0x0a, &Poser::stateA},          {0x0b, &Poser::stateB},          {0x0f, &Poser::swingF10},
    {0x10, &Poser::swingF10},        {0x11, &Poser::blow11_12},       {0x12, &Poser::blow11_12},
    {0x13, &Poser::states13_14},     {0x14, &Poser::states13_14},     {0x15, &Poser::state15},
    {0x16, &Poser::state16},         {0x17, &Poser::states17_1a_1c},  {0x18, &Poser::states18_19},
    {0x19, &Poser::states18_19},     {0x1a, &Poser::states17_1a_1c},  {0x1b, &Poser::state1b},
    {0x1c, &Poser::states17_1a_1c},  {0x1d, &Poser::state1d},         {0x1e, &Poser::state1e},
    {0x1f, &Poser::state1f},         {0x20, &Poser::swings20_31_32},  {0x21, &Poser::state21},
    {0x22, &Poser::state22},         {0x23, &Poser::state23},         {0x24, &Poser::state24},
    {0x26, &Poser::state26},         {0x27, &Poser::state27},         {0x28, &Poser::state28},
    {0x29, &Poser::state29},         {0x2a, &Poser::state2a},         {0x2b, &Poser::state2b},
    {0x2c, &Poser::state2c},         {0x2d, &Poser::bite2d_50},       {0x2e, &Poser::state2e},
    {0x2f, &Poser::state2f},         {0x30, &Poser::state30},         {0x31, &Poser::swings20_31_32},
    {0x32, &Poser::swings20_31_32},  {0x33, &Poser::state33},         {0x34, &Poser::using34_35},
    {0x35, &Poser::using34_35},      {0x36, &Poser::state36},         {0x37, &Poser::state37},
    {0x38, &Poser::sitting38_39},    {0x39, &Poser::sitting38_39},    {0x3a, &Poser::state3a},
    {0x3b, &Poser::hanging3b_3c},    {0x3c, &Poser::hanging3b_3c},    {0x3d, &Poser::state3d},
    {0x3e, &Poser::state3e},         {0x3f, &Poser::riding3f_40},     {0x40, &Poser::riding3f_40},
    {0x41, &Poser::state41},         {0x42, &Poser::stirring42_44},   {0x43, &Poser::stirring42_44},
    {0x44, &Poser::stirring42_44},   {0x45, &Poser::state45},         {0x47, &Poser::state47},
    {0x48, &Poser::state48},         {0x49, &Poser::state49},         {0x4a, &Poser::state4a},
    {0x4b, &Poser::state4b},         {0x4c, &Poser::state4c},         {0x4d, &Poser::state4d},
    {0x4e, &Poser::eating4e_4f},     {0x4f, &Poser::eating4e_4f},     {0x50, &Poser::bite2d_50},
    {0x51, &Poser::state51},         {0x52, &Poser::state52},         {0x54, &Poser::state54},
    {0x55, &Poser::state55},         {0x56, &Poser::state56},         {0x57, &Poser::circling57_58},
    {0x58, &Poser::circling57_58},   {0x59, &Poser::state59},         {0x5a, &Poser::busy5a_62},
    {0x5b, &Poser::busy5a_62},       {0x5c, &Poser::busy5a_62},       {0x5d, &Poser::busy5a_62},
    {0x5e, &Poser::busy5a_62},       {0x5f, &Poser::busy5a_62},       {0x60, &Poser::busy5a_62},
    {0x61, &Poser::state61},         {0x62, &Poser::busy5a_62},       {0x63, &Poser::state63},
    {0x64, &Poser::swaying64_66},    {0x65, &Poser::swaying64_66},    {0x66, &Poser::swaying64_66},
};

void Poser::stateTargets() {
    const int st = state();
    for (const StateRow &row : STATES)
        if (row.state == st) {
            (this->*row.entry)();
            return;
        }
}
