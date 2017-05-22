#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace baidu {
namespace dit {

std::string generate_uuid() {
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    const std::string tmp = boost::uuids::to_string(uuid);
    return tmp;
}

}
}

