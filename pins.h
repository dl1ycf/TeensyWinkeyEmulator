//
// Hardware lines.
// For simple Arduinos, you cannot use 0 and 1 (hardware serial line),
// and you can only use 2 and 3 for Paddle (since they should trigger interrupts)
//

#define PaddleRight         0      // input for right paddle
#define PaddleLeft          1      // input for left paddle
#define StraightKey         2      // input for straight key (only used with the STRAIGHT_KEY feature)

#define CWOUT               9      // CW keying output (active high)
#define PTTOUT              13     // PTT output (active high) (and built-in LED)

#define POTPIN              A0     // analog input pin for the Speed pot, do not define if not connected
#define VOLPIN              A1     // analog input pin for the volume pot, do not define if not connected

#define RSPIN               3      // LCD display RS (only used with the LCDDISPLAY feature)
#define ENPIN               4      // LCD display EN (only used with the LCDDISPLAY feature)
#define D4PIN               5      // LCD display D4 (only used with the LCDDISPLAY feature)
#define D5PIN               6      // LCD display D5 (only used with the LCDDISPLAY feature)
#define D6PIN               7      // LCD display D6 (only used with the LCDDISPLAY feature)
#define D7PIN               8      // LCD display D7 (only used with the LCDDISPLAY feature)
