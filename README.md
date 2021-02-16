# TeensyWinkeyEmulator

Winkey (K1EL) emulator, can key SDR programs via MIDI and acts as a soundcard (for side tone mixing)

Using a Teensy 4.0, this will show up at the computer as THREE devices:

- serial interface: implementing the Winkey 2.1 protocol
- MIDI controller:  for sending key-down/key-up and PTT-on/PTT-off messages to the computer via MIDI
- sound card:       for RX audio from SDR programs, this will be mixed with the keyer side tone

The Audio part has originally been designed and implemented by Steve Haynal.


