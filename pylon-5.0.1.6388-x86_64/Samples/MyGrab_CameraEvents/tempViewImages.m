% tempViewImage.m


%dirRoot = './captures1';
%dirRoot = './captures_mono8';
dirRoot = './captures_mono12p';
%dirRoot = '/home/mroos/scratch/';

%d = dir([dirRoot '/*.png']);
d = dir([dirRoot '/*.tiff']);
%d = dir([dirRoot '/*.bmp']);

nPics = length(d);

figure(1); clf;
colormap gray;

pixelFormat = {'Mono 8','Mono 12','Mono 12p'};
bits = [8 12 12];

%for i = 1:nPics
for i = 1:3
   im = double(imread(fullfile(dirRoot,d(i).name)));
%    if bits(i)==12 && isequal(d(i).name(end-3:end),'tiff');
%       im = im / 2^4;
%    end
   subplot(3,2,(i-1)*2+1);
   imagesc(im); colorbar;
   title(sprintf('%s%s: %d of %d',dirRoot,d(i).name,i,nPics),'interpreter','none');
%    title(sprintf('Format: %s',pixelFormat{i}));

   subplot(3,2,(i-1)*2+2);

   P = 'cdf';  % cdf or hist
   switch P
      case 'hist'
         %binCenters = 0:(2^bits(i)-1);
         %binCenters = min(im(:)):max(im(:));
         binCenters = linspace(min(im(:)),max(im(:)),50);
         cnt = hist(im(:),binCenters);
         bar(binCenters,cnt); gz;
         set(gca,'YScale','log');
         xlabel('Pixel level');
         ylabel('Pixel count (log scale)');
      case 'cdf'
         plot(sort(im(:)));
         xlabel('Pixel number');
         ylabel('Pixel level');
         set(gca,'YScale','log');
   end
   
   nLevels = length(unique(im(:)));
   title(sprintf('%d levels (%0.2f bits)',nLevels,log2(nLevels)));
   %v=axis; axis([min(im(:))+[1 40] v(3:4)]);
   
   
   
end

