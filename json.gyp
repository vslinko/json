{
    "targets": [
        {
            "target_name": "json",
            "type": "executable",
            "sources": [
                "src/main.c",
                "src/json.c",
                "src/json_template.c"
            ]
        },
        {
            "target_name": "json-test",
            "type": "executable",
            "include_dirs": [
                "vendor/vstd/src"
            ],
            "sources": [
                "vendor/vstd/src/vstd/test_runner.c",
                "vendor/vstd/src/vstd/test.c",
                "src/_json_test.c",
                "src/json.c"
            ]
        }
    ]
}
