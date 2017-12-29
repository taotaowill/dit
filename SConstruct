protoc = Builder(action='thirdparty/bin/protoc --proto_path=./src/proto/  --cpp_out=./src/proto/ $SOURCE')
env_gen = Environment(BUILDERS={'Protoc': protoc})
env_gen.Protoc(['src/proto/dit.pb.h','src/proto/dit.pb.cc'], 'src/proto/dit.proto')

env = Environment(
        CPPPATH = ['thirdparty/boost_1_57_0/', './thirdparty/include', '.', 'src'] ,
        LIBS = ['glog', 'gflags', 'gtest', 'tcmalloc', 'unwind', 'pthread', 'rt', 'boost_filesystem', 'ins_sdk','sofa-pbrpc', 'protobuf', 'z', 'snappy',  'common'],
        LIBPATH = ['./thirdparty/lib', './thirdparty/boost_1_57_0/stage/lib'],
        CCFLAGS = '-g2 -Wall -Wno-unused-function -Wno-deprecated-declarations -Wno-unused-but-set-variable',
        CPPDEFINES=[('private','public')],
        LINKFLAGS = '-Wl,-rpath-link ./thirdparty/boost_1_57_0/stage/lib')

env.Program('ditd', Glob('src/server/*.cc') + ['src/dit_flags.cc', 'src/proto/dit.pb.cc'])
env.Program('dit', Glob('src/client/*.cc') + ['src/dit_flags.cc', 'src/proto/dit.pb.cc'])

# unittest
env.Program('test_client', ['test/client/test_client.cc', 'src/dit_flags.cc', 'src/client/dit_client.cc', 'src/proto/dit.pb.cc'])
env.Program('test_common_util', ['test/utils/test_common_util.cc'])

