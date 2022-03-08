ipcs -s | grep icecool | awk ' { print $2 } ' | xargs ipcrm sem

