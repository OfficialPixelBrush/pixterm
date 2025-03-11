// RGB Color Struct going from 0.0f - 1.0f
struct Color {
	float r;
	float g;
	float b;
};
typedef struct Color Color;

/* --- Default Color ---- */
Color white  = {1.0f, 1.0f, 1.0f};
Color black  = {0.0f, 0.0f, 0.0f};

Color red    = {1.0f, 0.0f, 0.0f};
Color green  = {0.0f, 1.0f, 0.0f};
Color blue   = {0.0f, 0.0f, 1.0f};

Color yellow = {1.0f, 1.0f, 0.0f};
Color magenta = {1.0f, 0.0f, 1.0f};
Color cyan   = {0.0f, 1.0f, 1.0f};