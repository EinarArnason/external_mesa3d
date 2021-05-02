mkdir -p prebuilt-intermediates/{glsl,main,nir,spirv,cle,v3dv,vulkan,util}

python src/compiler/glsl/ir_expression_operation.py strings > prebuilt-intermediates/glsl/ir_expression_operation_strings.h
python src/compiler/glsl/ir_expression_operation.py constant > prebuilt-intermediates/glsl/ir_expression_operation_constant.h
python src/compiler/glsl/ir_expression_operation.py enum > prebuilt-intermediates/glsl/ir_expression_operation.h

python src/mesa/main/format_pack.py  src/mesa/main/formats.csv  > prebuilt-intermediates/main/format_pack.c
python src/mesa/main/format_fallback.py  src/mesa/main/formats.csv /dev/stdout  > prebuilt-intermediates/main/format_fallback.c

python src/compiler/nir/nir_builder_opcodes_h.py src/compiler/nir/nir_opcodes.py > prebuilt-intermediates/nir/nir_builder_opcodes.h
python src/compiler/nir/nir_constant_expressions.py src/compiler/nir/nir_opcodes.py > prebuilt-intermediates/nir/nir_constant_expressions.c
python src/compiler/nir/nir_opcodes_c.py src/compiler/nir/nir_opcodes.py > prebuilt-intermediates/nir/nir_opcodes.c
python src/compiler/nir/nir_opcodes_h.py src/compiler/nir/nir_opcodes.py > prebuilt-intermediates/nir/nir_opcodes.h
python src/compiler/nir/nir_opt_algebraic.py src/compiler/nir/nir_opt_algebraic.py > prebuilt-intermediates/nir/nir_opt_algebraic.c
python src/compiler/nir/nir_intrinsics_c.py --outdir prebuilt-intermediates/nir/ || ( prebuilt-intermediates/nir/nir_intrinsics.c; false)
python src/compiler/nir/nir_intrinsics_h.py --outdir prebuilt-intermediates/nir/ || ( prebuilt-intermediates/nir/nir_intrinsics.h; false)
python src/compiler/nir/nir_intrinsics_indices_h.py --outdir prebuilt-intermediates/nir/ || ( prebuilt-intermediates/nir/nir_intrinsics_indices.h; false)

python src/compiler/spirv/spirv_info_c.py src/compiler/spirv/spirv.core.grammar.json prebuilt-intermediates/spirv/spirv_info.c || ( prebuilt-intermediates/spirv/spirv_info.c; false)
python src/compiler/spirv/vtn_gather_types_c.py src/compiler/spirv/spirv.core.grammar.json prebuilt-intermediates/spirv/vtn_gather_types.c || ( prebuilt-intermediates/spirv/vtn_gather_types.c; false)
python src/compiler/spirv/vtn_generator_ids_h.py src/compiler/spirv/spir-v.xml prebuilt-intermediates/spirv/vtn_generator_ids.h || ( prebuilt-intermediates/spirv/vtn_generator_ids.h; false)

python src/intel/genxml/gen_zipped_file.py src/broadcom/cle/v3d_packet_v21.xml src/broadcom/cle/v3d_packet_v33.xml > prebuilt-intermediates/cle/v3d_xml.h

python src/broadcom/cle/gen_pack_header.py src/broadcom/cle/v3d_packet_v21.xml 21 > prebuilt-intermediates/cle/v3d_packet_v21_pack.h
python src/broadcom/cle/gen_pack_header.py src/broadcom/cle/v3d_packet_v33.xml 33 > prebuilt-intermediates/cle/v3d_packet_v33_pack.h
python src/broadcom/cle/gen_pack_header.py src/broadcom/cle/v3d_packet_v33.xml 41 > prebuilt-intermediates/cle/v3d_packet_v41_pack.h
python src/broadcom/cle/gen_pack_header.py src/broadcom/cle/v3d_packet_v33.xml 42 > prebuilt-intermediates/cle/v3d_packet_v42_pack.h

python src/vulkan/util/vk_entrypoints_gen.py --xml src/vulkan/registry/vk.xml --proto --weak --prefix v3dv --out-h prebuilt-intermediates/v3dv/v3dv_entrypoints.h --out-c prebuilt-intermediates/v3dv/v3dv_entrypoints.c
python src/broadcom/vulkan/v3dv_extensions_gen.py --xml src/vulkan/registry/vk.xml --out-h prebuilt-intermediates/v3dv/v3dv_extensions.h
python src/broadcom/vulkan/v3dv_extensions_gen.py --xml src/vulkan/registry/vk.xml --out-c prebuilt-intermediates/v3dv/v3dv_extensions.c

python src/vulkan/util/gen_enum_to_str.py --xml src/vulkan/registry/vk.xml --outdir prebuilt-intermediates/vulkan/
python src/vulkan/util/vk_entrypoints_gen.py --xml src/vulkan/registry/vk.xml --proto --weak --prefix vk_common --out-h prebuilt-intermediates/vulkan/vk_common_entrypoints.h --out-c prebuilt-intermediates/vulkan/vk_common_entrypoints.c
python src/vulkan/util/vk_dispatch_table_gen.py --xml src/vulkan/registry/vk.xml --out-h prebuilt-intermediates/vulkan/vk_dispatch_table.h
python src/vulkan/util/vk_dispatch_table_gen.py --xml src/vulkan/registry/vk.xml --out-c prebuilt-intermediates/vulkan/vk_dispatch_table.c
python src/vulkan/util/vk_extensions_gen.py --xml src/vulkan/registry/vk.xml --out-h prebuilt-intermediates/vulkan/vk_extensions.h
python src/vulkan/util/vk_extensions_gen.py --xml src/vulkan/registry/vk.xml --out-c prebuilt-intermediates/vulkan/vk_extensions.c

python src/util/driconf_static.py src/util/00-mesa-defaults.conf prebuilt-intermediates/util/driconf_static.h
python3 src/gallium/auxiliary/util/u_tracepoints.py -p src/gallium/auxiliary/util -C prebuilt-intermediates/util/u_tracepoints.c -H prebuilt-intermediates/util/u_tracepoints.h
