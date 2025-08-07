idf_component_register(
    SRCS 
    "main.c"
    "../components/ui/my_global_lib.c"
    "../components/ui/transaction.c"
    "../components/ui/usr_k2_lan.c"
    "../components/ui/safe_page.c"
    "../components/ui/ui.c"
    "../components/ui/ui_helpers.c"
    
    INCLUDE_DIRS "."
)
