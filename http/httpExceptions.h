//
// Created by Дима Щипицын on 15/01/2022.
//

#ifndef WEBSERV_HTTPEXCEPTIONS_H
#define WEBSERV_HTTPEXCEPTIONS_H

#include <string>


struct httpExBase : public std::exception
{
    explicit httpExBase(const std::string& err) : error(err) {
        error_code = 0;
    }
    virtual ~httpExBase() _NOEXCEPT {

    }

    virtual const char* what() const _NOEXCEPT{
        return error.c_str();
    }
    std::string error;
    int         error_code;
};

template<class T>
struct httpEx : public httpExBase
{
    explicit httpEx(const std::string& err) : httpExBase(err) {
        error_code = T::error_code;
    }
    virtual ~httpEx() _NOEXCEPT {

    }
};

struct Unauthorized {enum {error_code = 401};};
struct BadRequest {enum {error_code = 400};};
struct PaymentRequired {enum {error_code = 402};};
struct Forbidden {enum {error_code = 403};};
struct NotFound {enum {error_code = 404};};
struct MethodNotAllowed {enum {error_code = 405};};
struct NotAcceptable {enum {error_code = 406};};
struct ProxyAuthenticationRequired {enum {error_code = 407};};
struct RequestTimeout {enum {error_code = 408};};
struct Conflict {enum {error_code = 409};};
struct Gone {enum {error_code = 410};};
struct LengthRequired {enum {error_code = 411};};
struct PreconditionFailed {enum {error_code = 412};};
struct PayloadToLarge {enum {error_code = 413};};
struct UriTooLong {enum {error_code = 414};};
struct UnsupportedMediaType {enum {error_code = 415};};
struct RangeNotSatisfiable {enum {error_code = 416};};
struct ExpectationFailed {enum {error_code = 417};};
struct ImATeapot {enum {error_code = 418};};
struct UnprocessableEntity {enum {error_code = 422};};
struct TooEarly {enum {error_code = 425};};
struct UpgradeRequired {enum {error_code = 426};};
struct PreconditionRequired {enum {error_code = 428};};
struct TooManyRequests {enum {error_code = 429};};
struct RequestHeaderFieldsTooLarge {enum {error_code = 431};};
struct UnavailableForLegalReasons {enum {error_code = 451};};

struct InternalServerError {enum {error_code = 500};};
struct NotImplemented {enum {error_code = 501};};
struct BadGateway {enum {error_code = 502};};
struct ServiceUnavailable {enum {error_code = 503};};
struct GatewayTimeout {enum {error_code = 504};};
struct HTTPVersionNotSupported {enum {error_code = 505};};
struct VariantAlsoNegotiates {enum {error_code = 506};};
struct InsufficientStorage {enum {error_code = 507};};
struct LoopDetected {enum {error_code = 508};};
struct NotExtended {enum {error_code = 510};};
struct NetworkAuthenticationRequired {enum {error_code = 511};};

#endif //WEBSERV_HTTPEXCEPTIONS_H
