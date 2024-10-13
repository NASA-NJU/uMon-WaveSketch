tbl = readtable('report7.csv','VariableNamingRule','preserve');
tbl.class = categorical(tbl.class);
%tbl.memory = categorical(tbl.memory);
tbl = tbl(tbl.length>=32,:);

[~,id] = findgroups(tbl.memory);
for m = 1:length(id)
    subtbl = tbl(tbl.memory==id(m),:);
    vars = subtbl.Properties.VariableNames(end-8:end);
    for v = 1:length(vars)
        scatter_plot(subtbl,vars{v});
    end
end

flow = readtable('sample0.csv');
flow.class = categorical(flow.class);

figure
h = gscatter(flow.time,flow.data,flow.class,[],[],14);
set(h,'LineStyle','-');
title("flow sample");

function [] = scatter_plot(t,label)
    figure
    %gscatter(t.('length'),t.(label),t.('class'),[],[],14)
    scatterhist(t.('length'),t.(label),'Group',t.('class'),'Marker','.','MarkerSize',14,'Kernel','on','Location','NorthEast','Direction','out');
    set(gca, 'XScale', 'log')
    title(label + "-" + t.memory(1) + "B")
    legend
end
