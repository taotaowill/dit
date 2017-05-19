protoc = Builder(action='thirdparty/bin/protoc --proto_path=./src/proto/  --cpp_out=./src/proto/ $SOURCE')
env_gen = Environment(BUILDERS={'Protoc': protoc})
env_gen.Protoc(['src/proto/dit.pb.h','src/proto/dit.pb.cc'], 'src/proto/dit.proto')

env = Environment(
        CPPPATH = ['thirdparty/boost_1_57_0/', './thirdparty/include', '.', 'src', 'src/client', 'src/server/'] ,
        LIBS = ['protobuf', 'sofa-pbrpc', 'glog', 'gflags', 'tcmalloc', 'unwind', 'pthread', 'z', 'rt', 'boost_filesystem', 'gtest', 'common', 'snappy'],
        LIBPATH = ['./thirdparty/lib', './thirdparty/boost_1_57_0/stage/lib'],
        CCFLAGS = '-g2 -Wall -Werror -Wno-unused-but-set-variable',
        LINKFLAGS = '-Wl,-rpath-link ./thirdparty/boost_1_57_0/stage/lib')

env.Program('ditd', Glob('src/server/*.cc') + ['src/proto/dit.pb.cc'])
env.Program('dit', Glob('src/client/*.cc'))

#unittest
