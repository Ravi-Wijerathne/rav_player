import sys
def get_rgb(Y, U, V):
    R = Y + 1.5958 * V
    G = Y - 0.39173 * U - 0.81290 * V
    B = Y + 2.017 * U
    return max(0, min(1, R)), max(0, min(1, G)), max(0, min(1, B))

# Skin
Y, U, V = 0.70, -0.125, 0.143

print(f"Normal: {get_rgb(Y, U, V)}")
print(f"UV Swap: {get_rgb(Y, V, U)}")
print(f"Inverted U: {get_rgb(Y, -U, V)}")
print(f"Inverted V: {get_rgb(Y, U, -V)}")
print(f"Inverted UV: {get_rgb(Y, -U, -V)}")
print(f"U=V: {get_rgb(Y, V, V)}")
print(f"V=U: {get_rgb(Y, U, U)}")
print(f"R/B Swap: {get_rgb(Y, U, V)[2]}, {get_rgb(Y, U, V)[1]}, {get_rgb(Y, U, V)[0]}")
print(f"G/B Swap: {get_rgb(Y, U, V)[0]}, {get_rgb(Y, U, V)[2]}, {get_rgb(Y, U, V)[1]}")
