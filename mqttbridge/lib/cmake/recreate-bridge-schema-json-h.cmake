function(make_includable INPUT_FILE OUTPUT_FILE VAR_NAME)
    file(READ ${INPUT_FILE} CONTENT)
    string(REGEX REPLACE "[ \t\r\n]" "" CONTENT "${CONTENT}")
    set(CONTENT "const std::string mqtt::bridge::lib::BridgeConfigLoader::${VAR_NAME} = R\"(${CONTENT})\";\n")
    file(WRITE ${OUTPUT_FILE} "${CONTENT}")
endfunction(make_includable)

if(bridge-schema.json IS_NEWER_THAN ${TARGET_PATH}/bridge-schema.json.h)
    make_includable(bridge-schema.json ${TARGET_PATH}/bridge-schema.json.h bridgeJsonSchemaString)
endif(bridge-schema.json IS_NEWER_THAN ${TARGET_PATH}/bridge-schema.json.h)
