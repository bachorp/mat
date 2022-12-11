library(hexbin)
library(RColorBrewer)

makespans <- function(variant) {
    result <- list()
    for (g in 4:12) {
        for (s in 0:19) {
            result <- c(result, unlist(read.csv(paste(
                "input/", variant, "/", g, "/", s, ".csv",
                sep = ""
            ))["makespan"]))
        }
    }
    return(result)
}

mat <- makespans("mat")
non_blocking <- makespans("non_blocking")
fixed <- makespans("fixed")
mapd <- makespans("mapd")

colors <- colorRampPalette(rev(brewer.pal(11, "Spectral")))

bin <- hexbin(mat, mapd, xbins = 30)
plot(bin, colramp = colors)
