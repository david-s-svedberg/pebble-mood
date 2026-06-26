#pragma once

// Exports registrations to the phone over AppMessage so a companion app can
// pull them for correlation analysis. Registrations are sent one per message,
// advancing on each outbox ack. The phone kicks this off by sending an
// EXPORT_REQUEST (see src/pkjs/index.js).
void data_export_init();
void data_export_send_all();
