# Canonical Call Flows

Status: Accepted for header scaffolding
Phase: 01

## Purpose

Show the intended usage patterns for the first public API so the contract is concrete before headers are written.

## Flow 1: Generic query

Use this for commands like `name,?`, `status`, and other non-read command/response paths.

```c
ezo_i2c_device_t device;
ezo_timing_hint_t hint;
char response[32];
size_t response_len = 0;
ezo_device_status_t status = EZO_STATUS_UNKNOWN;
ezo_result_t result;

result = ezo_device_init(&device, 0x63, transport, context);
if (result != EZO_OK) {
  /* handle init error */
}

result = ezo_send_command(&device, "name,?", EZO_COMMAND_GENERIC, &hint);
if (result != EZO_OK) {
  /* handle send failure */
}

/* caller waits hint.wait_ms */

result = ezo_read_response(
    &device,
    response,
    sizeof(response),
    &response_len,
    &status);

if (result != EZO_OK) {
  /* handle library failure */
}

if (status != EZO_STATUS_SUCCESS) {
  /* handle device-reported unsuccessful state */
}
```

## Flow 2: Read and parse numeric result

Use this for simple numeric read responses.

```c
double reading = 0.0;

result = ezo_send_read(&device, &hint);
if (result != EZO_OK) {
  /* handle send failure */
}

/* caller waits hint.wait_ms */

result = ezo_read_response(
    &device,
    response,
    sizeof(response),
    &response_len,
    &status);

if (result != EZO_OK) {
  /* handle library failure */
}

if (status != EZO_STATUS_SUCCESS) {
  /* handle device status */
}

result = ezo_parse_double(response, response_len, &reading);
if (result != EZO_OK) {
  /* handle parse failure */
}
```

## Flow 3: Temperature-compensated read

Use this when the command itself encodes a numeric parameter and behaves like a read path.

```c
result = ezo_send_read_with_temp_comp(&device, 25.0, 3, &hint);
if (result != EZO_OK) {
  /* handle send failure */
}

/* caller waits hint.wait_ms */

result = ezo_read_response(
    &device,
    response,
    sizeof(response),
    &response_len,
    &status);
```

## Flow 4: Device status not ready

This is not a library failure.

```c
result = ezo_read_response(..., &status);

if (result == EZO_OK && status == EZO_STATUS_NOT_READY) {
  /* caller decides whether and how to retry */
}
```

## Flow 5: Transport failure

This is a library failure, not a device status.

```c
result = ezo_read_response(..., &status);

if (result == EZO_ERR_TRANSPORT) {
  /* transport failed; status is not authoritative */
}
```

## Design Notes

1. The API remains generic.
2. Timing stays caller-controlled.
3. Parsing is explicit.
4. Device-reported unsuccessful states are not collapsed into transport or parse failures.
5. The API does not preserve the legacy `NOT_READ_CMD` object-state model.

