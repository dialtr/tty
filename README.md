# tty

This is a simple program that opens a serial port device for communication,
initializing it to useful defaults (8 data bits, 1 stop bit, no parity) for
1200 baud communication.

Note: this program was entirely generated with Google Gemini AI. The
implementation is naive in the sense that it busy polls the descriptor, so
it should not be used in a production system.
