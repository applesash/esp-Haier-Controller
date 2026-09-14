# Haier Roomstat Modbus Investigation

## Operating rules

The controller starts in passive observation mode. Do not transmit writes to the Haier bus until the electrical interface, serial format, device address, and register semantics are independently verified.

Capture one physical action at a time and record:

- timestamp and direction
- baud rate, parity, and stop bits
- raw bytes and CRC result
- device address, function, register, and quantity
- physical action that preceded the frame
- interpretation and confidence level

Use confidence labels: `confirmed`, `strongly inferred`, `tentative`, or `unknown`.

## First investigation sequence

1. Verify that the HVAC interface is electrically compatible with isolated RS485.
2. Observe the idle bus without transmitting.
3. Identify frame timing, serial format, and likely address.
4. Correlate traffic with a single Roomstat action.
5. Repeat the action to separate periodic polling from event-driven frames.
6. Implement read-only decoding only after repeated evidence.
7. Keep writes disabled until the device behavior and recovery path are documented.

## Room readings

The initial application model will expose two independently mapped room sources:

- `upstairs`
- `downstairs`

A source must carry availability, freshness, last-update time, error count, bus address, and register identity. A numeric temperature without that metadata is not considered trustworthy for control.
