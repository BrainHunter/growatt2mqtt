Import("env")
env.Replace(UPLOADCMD="curl -u admin:growatt2mqttPassword -v -F image=@$SOURCE $UPLOAD_PORT")
