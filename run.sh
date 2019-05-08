#!/bin/bash
 for i in {1902..1905}
 	do 
	Debug/client-ss-service localhost:$i default default &
        sleep 0.1
	done
