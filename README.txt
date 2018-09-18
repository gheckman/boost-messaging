╔══════════════════════════════════════════════════════════════════════════════╗
║ Concepts                                                                     ║
╚══════════════════════════════════════════════════════════════════════════════╝

Classes that must adhere to certain concepts can find more information on those
specific concepts below.

┌──────────────────────────────────────────────────────────────────────────────┐
│ Serializer                                                                   │
└──────────────────────────────────────────────────────────────────────────────┘

The things Serializer needs to define
FwdIter can be any forward iterator type, including pointers

typedef something send_t
typedef something recv_t

size_t header_size()

template <typename FwdIter>
size_t body_size(FwdIter first, FwdIter last)

template <typename FwdIter>
bool validate_header(FwdIter first, FwdIter last)

std::vector<char> serialize(const send_t& send_msg)

template <typename FwdIter>
recv_t deserialize(FwdIter first, FwdIter last)

┌──────────────────────────────────────────────────────────────────────────────┐
│ Handler                                                                      │
└──────────────────────────────────────────────────────────────────────────────┘

The things Handler needs to define
T is the recv_t from Serializer

template <typename T>
void handle(const T& t)