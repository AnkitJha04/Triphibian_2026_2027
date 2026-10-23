"""Reference-vector check for the MS5607 datasheet compensation equations."""

def compensate(c, d1, d2):
    dt = d2 - (c[5] << 8)
    temp = 2000 + (dt * c[6]) // (1 << 23)
    off = (c[2] << 17) + (c[4] * dt) // (1 << 6)
    sens = (c[1] << 16) + (c[3] * dt) // (1 << 7)
    t2 = off2 = sens2 = 0
    if temp < 2000:
        t2 = (dt * dt) >> 31
        t = temp - 2000
        off2 = 61 * t * t // 16
        sens2 = 2 * t * t
        if temp < -1500:
            t2low = temp + 1500
            off2 += 15 * t2low * t2low
            sens2 += 8 * t2low * t2low
    temp -= t2
    off -= off2
    sens -= sens2
    pressure_pa = (((d1 * sens) // (1 << 21)) - off) // (1 << 15)
    return temp / 100, pressure_pa

# Datasheet example coefficients and ADC values: 20.00 C, 1100.02 mbar = 110002 Pa.
c = [0, 46372, 43981, 29059, 27842, 31553, 28165, 0]
t, p = compensate(c, 6465444, 8077636)
assert abs(t - 20.0) < 0.01, t
assert abs(p - 110002) <= 2, p
print("MS5607 datasheet reference-vector test passed")
