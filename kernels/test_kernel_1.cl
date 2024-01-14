kernel void test_kernel_1(global int * result) {
    result[get_global_id(0)] *= get_global_id(0);
}
