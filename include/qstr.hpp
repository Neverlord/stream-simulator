#ifndef QSTR_HPP
#define QSTR_HPP

#include <string>
#include <type_traits>

#include <QString>

/// Convenience function for calling `QString::fromUtf8()`.
inline QString qstr(const char* cstr) {
  return QString::fromUtf8(cstr);
}

/// Convenience function for printing addresses of pointers.
inline QString qstr(const void* vptr) {
  return QString("0x%1")
         .arg(reinterpret_cast<quintptr>(vptr),
              QT_POINTER_SIZE * 2, 16, QChar('0'));
}

/// Alias function for converting `std::string` to QString.
inline QString qstr(const std::string& str) {
  return QString::fromStdString(str);
}

/// Convenience function for calling `QString::number()`.
template <class T, class E = std::enable_if_t<std::is_integral<T>::value>>
QString qstr(T x, int base = 10) {
  return QString::number(x, base);
}
#endif // QSTR_HPP
