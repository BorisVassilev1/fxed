import os

os.makedirs("shadercache", exist_ok=True)
print("// Auto-generated shader registry")
print("#include \"shader_registry.hpp\"")
for file in os.listdir("shadercache"):
    if file.endswith(".spv"):
        name = file.replace(".spv", "")
        print(f'constexpr unsigned char {name}_spv[] = {{')
        with open(os.path.join("shadercache", file), "rb") as f:
            byte = f.read(1)
            count = 0
            while byte:
                print(f'    0x{byte.hex()},', end="")
                count += 1
                if count % 12 == 0:
                    print()
                byte = f.read(1)
        print("\n};")
        print(f"constexpr size_t {name}_spv_len = sizeof({name}_spv);")

print("\nstd::unordered_map<std::string, ShaderData> g_shaders = {")
for file in os.listdir("shadercache"):
    if file.endswith(".spv"):
        name = file.replace(".spv", "")
        name2 = name.replace(".", "_")
        print(f'    {{"{name}.spv", {{{name2}_spv, {name2}_spv_len}}}},')
print("};")
