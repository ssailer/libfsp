fsp_test_record_sizes = executable('fsp_test_record_sizes', 'fsp_test_record_sizes.c', dependencies : [fsp_dep])
fsp_test_record_consistency = executable('fsp_test_record_consistency', 'fsp_test_record_consistency.c', dependencies : [fsp_dep])

test('fsp_test_record_sizes', fsp_test_record_sizes, is_parallel : true, suite : ['fsp'], args : ['1'])
test('fsp_test_record_consistency', fsp_test_record_consistency, is_parallel : true, suite : ['fsp'], args : ['fsp_test_record_consistency.dat'])
